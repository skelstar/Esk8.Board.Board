#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

SemaphoreHandle_t mux_I2C;
SemaphoreHandle_t mux_SPI;

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

#include <tasks/queues/queues.h>

#include <RF24Network.h>
#include <NRF24L01Lib.h>
#include <GenericClient.h>

#include <QueueManager.h>

#include <tasks/root.h>

#include <Fsm.h>

elapsedMillis
    sinceLastControllerPacket,
    sinceBoardBooted,
    sinceUpdatedButtonAValues,
    sinceNRFUpdate;

bool controller_connected = false;

//------------------------------------------------------------------

VescData board_packet;

// NRF24L01Lib nrf24;

// RF24 radio(SPI_CE, SPI_CS);
// RF24Network network(radio);

// GenericClient<VescData, ControllerData> controllerClient(COMMS_CONTROLLER);

void send_to_vesc(uint8_t throttle, bool cruise_control);

// #include <controller_comms.h>

// const char *getSummary(VescData d)
// {
//   char buff[50];
//   sprintf(buff, "id: %lu moving: %d", d.id, d.moving);
//   return buff;
// }

// const char *getSummary(ControllerData d)
// {
//   char buff[50];
//   sprintf(buff, "id: %lu throttle: %d", d.id, d.throttle);
//   return buff;
// }

// void controllerClientInit()
// {
//   // TODO: PRINT_FLAGS
//   controllerClient.begin(&network, controllerPacketAvailable_cb);
//   controllerClient.setConnectedStateChangeCallback([] {
//     Serial.printf("setConnectedStateChangeCallback\n");
//   });
//   controllerClient.setSentPacketCallback([](VescData data) {
//     if (PRINT_TX_TO_CONTROLLER)
//       Serial.printf(PRINT_TX_PACKET_TO_FORMAT, "CTRLR", getSummary(data));
//   });
//   controllerClient.setReadPacketCallback([](ControllerData data) {
//     if (PRINT_RX_FROM_CONTROLLER)
//       Serial.printf(PRINT_RX_PACKET_FROM_FORMAT, "CTRLR", getSummary(data));
//   });
// }

#define NUM_RETRIES 5

//------------------------------------------------------------------

// #include <peripherals.h>
// #if (FEATURE_FOOTLIGHT == 1)
// #include <tasks/core_0/footLightTask_0.h>
// #endif
// #include <tasks/core_0/buttonsTask.h>
// #include <tasks/core_0/commsFsmTask.h>
// #include <vesc_comms_2.h>

// void waitForTasksToBeReady();

void createQueues();
void createLocalQueueManagers();
void startTasks();
void configureTasks();
void waitForTasks();
void enableTasks(bool print = false);

//============================================================

void setup()
{
  Serial.begin(115200);

  board_packet.version = VERSION;

  //get chip id
  String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
  chipId.toUpperCase();

  createQueues();

  print_build_status(chipId);

  if (boardIs(chipId, M5STACKFIREID) && USING_M5STACK == 1)
  {
    DEBUG("-----------------------------------------------");
    DEBUG("               USING_M5STACK                   ");
    DEBUG("-----------------------------------------------\n\n");
  }

  if (boardIs(chipId, TDISPLAYBOARD))
  {
    DEBUG("-----------------------------------------------");
    DEBUG("               T-DISPLAY BOARD              ");
    DEBUG("-----------------------------------------------\n\n");
  }

  configureTasks();

  startTasks();

  waitForTasks();

  enableTasks();
}

elapsedMillis sinceCheckedCtrlOnline;

void loop()
{
  // if (sinceNRFUpdate > 20)
  // {
  //   sinceNRFUpdate = 0;
  //   // controllerClient.update();
  // }

  // vesc_update();

  // TODO read from local "controller"
  // if (sinceCheckedCtrlOnline > 500 && controller.hasTimedout(sinceLastControllerPacket))
  // {
  //   sinceCheckedCtrlOnline = 0;
  //   // DEBUGMVAL("timeout", sinceLastControllerPacket);
  //   // Comms::commsFsm.trigger(Comms::EV_CTRLR_TIMEOUT);
  // }

  vTaskDelay(10);
}

//===================================================

void createQueues()
{
  xControllerQueueHandle = xQueueCreate(1, sizeof(ControllerClass *));
  xVescQueueHandle = xQueueCreate(1, sizeof(VescData *));
  xSimplMessageQueue = xQueueCreate(1, sizeof(SimplMessageObj *));
}

#include <tasks/queues/QueueFactory.h>

void createLocalQueueManagers()
{
}

void configureTasks()
{
  // ctrlrCommsTask.healthCheck = true;
  // vescCommsTask.healthCheck = true;
  // m5StackDisplayTask.healthCheck = true;
  // mockVescTask.healthCheck = true;
  vescCommsTask.doWorkInterval = PERIOD_100ms;
}

#define USE_M5STACK_DISPLAY 0
#define USE_M5STACK_BUTTONS 1

void startTasks()
{
  ctrlrCommsTask.start(nsControllerCommsTask::task1);
  vescCommsTask.start(nsVescCommsTask::task1);
  m5StackDisplayTask.start(nsM5StackDisplayTask::task1);
  mockVescTask.start(nsMockVescTask::task1);
}

elapsedMillis since_reported_waiting;

void waitForTasks()
{

  while (
      !ctrlrCommsTask.ready ||
      !vescCommsTask.ready ||
      !m5StackDisplayTask.ready ||
      !mockVescTask.ready ||
      false)
  {
    if (since_reported_waiting > PERIOD_500ms)
    {
      since_reported_waiting = 0;
      Serial.printf("Waiting for Tasks to be ready\n");
    }
    vTaskDelay(PERIOD_10ms);
  }
  Serial.printf("-- all tasks ready! --\n");
}

void enableTasks(bool print)
{
  ctrlrCommsTask.enable(print);
  vescCommsTask.enable(print);
  m5StackDisplayTask.enable(print);
  mockVescTask.enable(print);
}
