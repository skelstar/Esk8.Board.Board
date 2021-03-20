#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

#include <shared-utils.h>
#include <types.h>
#include <printFormatStrings.h>
#include <utils.h>
#include <FsmManager.h>
#include <QueueManager.h>
#include <constants.h>
#include <ControllerClass.h>

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI(LCD_HEIGHT, LCD_WIDTH);

ControllerClass controller;

#if USING_M5STACK
xQueueHandle xM5StackDisplayQueue;
Queue::Manager *displayQueue;
namespace M5StackDisplay
{
  void initQueue()
  {
    displayQueue = new Queue::Manager(/*len*/ 3, sizeof(uint16_t), /*ticks*/ 3);
    displayQueue->setName("m5StackDispQueue");
    displayQueue->setSentEventCallback([](uint16_t ev) {
      if (PRINT_DISP_QUEUE_SEND)
        Serial.printf("sent to displayQueue %s\n", queueEvent(ev));
    });
    displayQueue->setReadEventCallback([](uint16_t ev) {
      if (PRINT_DISP_QUEUE_READ)
        Serial.printf("read from displayQueue %s\n", queueEvent(ev));
    });
  }
} // namespace M5StackDisplay

#include <tasks/core_0/m5StackDisplayTask.h>
#endif

#include <RF24Network.h>
#include <NRF24L01Lib.h>
#include <GenericClient.h>
#include <QueueManager.h>

#include <Fsm.h>

elapsedMillis
    sinceLastControllerPacket,
    sinceBoardBooted,
    sinceUpdatedButtonAValues,
    sinceNRFUpdate;

bool controller_connected = false;

xQueueHandle xFootLightEventQueue;

Queue::Manager *footlightQueue;

namespace FootLightQueue
{
  void sendToQueue_cb(uint16_t ev)
  {
    if (PRINT_SEND_TO_FOOTLIGHT_QUEUE)
      Serial.printf(PRINT_QUEUE_SEND_FORMAT, "FootLight", FootLight::getEvent(ev));
  }
  void readFromQueue_cb(uint16_t ev)
  {
    if (PRINT_READ_FROM_FOOTLIGHT_QUEUE)
      Serial.printf(PRINT_QUEUE_READ_FORMAT, "FootLight", FootLight::getEvent(ev));
  }

  void init()
  {
    footlightQueue = new Queue::Manager(/*len*/ 3, sizeof(FootLight::Event), /*ticks*/ 5);
    footlightQueue->setName("footLightQueue");
    footlightQueue->setSentEventCallback(sendToQueue_cb);
    footlightQueue->setReadEventCallback(readFromQueue_cb);
  }
} // namespace FootLightQueue

//------------------------------------------------------------------

VescData board_packet;

NRF24L01Lib nrf24;

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

GenericClient<VescData, ControllerData> controllerClient(COMMS_CONTROLLER);

void send_to_vesc(uint8_t throttle, bool cruise_control);

#include <controller_comms.h>

const char *getSummary(VescData d)
{
  char buff[50];
  sprintf(buff, "id: %lu moving: %d", d.id, d.moving);
  return buff;
}

const char *getSummary(ControllerData d)
{
  char buff[50];
  sprintf(buff, "id: %lu throttle: %d", d.id, d.throttle);
  return buff;
}

void controllerClientInit()
{
  // TODO: PRINT_FLAGS
  controllerClient.begin(&network, controllerPacketAvailable_cb);
  controllerClient.setConnectedStateChangeCallback([] {
    Serial.printf("setConnectedStateChangeCallback\n");
  });
  controllerClient.setSentPacketCallback([](VescData data) {
    if (PRINT_TX_TO_CONTROLLER)
      Serial.printf(PRINT_TX_PACKET_TO_FORMAT, "CTRLR", getSummary(data));
  });
  controllerClient.setReadPacketCallback([](ControllerData data) {
    if (PRINT_RX_FROM_CONTROLLER)
      Serial.printf(PRINT_RX_PACKET_FROM_FORMAT, "CTRLR", getSummary(data));
  });
}

#define NUM_RETRIES 5

//------------------------------------------------------------------

#include <peripherals.h>
#include <tasks/core_0/footLightTask_0.h>
#include <tasks/core_0/buttonsTask.h>
#include <tasks/core_0/commsFsmTask.h>
#include <vesc_comms_2.h>

//-------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  board_packet.version = VERSION;

  nrf24.begin(&radio, &network, COMMS_BOARD, controllerPacketAvailable_cb);
  vesc.init(VESC_UART_BAUDRATE);

  //get chip id
  String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
  chipId.toUpperCase();

  controllerClientInit();

  controller.config.send_interval = 200;

  print_build_status(chipId);

  if (boardIs(chipId, M5STACKFIREID))
  {
    DEBUG("-----------------------------------------------");
    DEBUG("               USING_M5STACK                   ");
    DEBUG("-----------------------------------------------\n\n");

    m5StackButtons_init();

#if USING_M5STACK
    M5StackDisplay::createTask(CORE_0, TASK_PRIORITY_2);
    M5StackDisplay::initQueue();
#endif
  }

  if (boardIs(chipId, TDISPLAYBOARD))
  {
    DEBUG("-----------------------------------------------");
    DEBUG("               T-DISPLAY BOARD              ");
    DEBUG("-----------------------------------------------\n\n");
  }

  if (USING_M5STACK)
  {
    Buttons::createTask(CORE_0, TASK_PRIORITY_2);
  }
  Comms::createTask(CORE_0, TASK_PRIORITY_1);

  if (FEATURE_FOOTLIGHT)
  {
    FootLight::createTask(CORE_0, TASK_PRIORITY_3);
    FootLightQueue::init();
  }

  while (false == Comms::taskReady &&
         false == Buttons::taskReady &&
         false == FootLight::taskReady)
  {
    vTaskDelay(5);
  }

  // send startup packet
  sendPacketToController(FIRST_PACKET);
}

elapsedMillis sinceCheckedCtrlOnline;

void loop()
{
  if (sinceNRFUpdate > 20)
  {
    sinceNRFUpdate = 0;
    controllerClient.update();
  }

  vesc_update();

  if (sinceCheckedCtrlOnline > 500 && controller.hasTimedout(sinceLastControllerPacket))
  {
    sinceCheckedCtrlOnline = 0;
    // DEBUGMVAL("timeout", sinceLastControllerPacket);
    Comms::commsFsm.trigger(Comms::EV_CTRLR_TIMEOUT);
  }

  vTaskDelay(10);
}