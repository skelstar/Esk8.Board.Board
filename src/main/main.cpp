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
#include <SharedConstants.h>
#include <QueueManager.h>
#include <constants.h>
#include <macros.h>
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

// VescData board_packet;

// NRF24L01Lib nrf24;

// RF24 radio(SPI_CE, SPI_CS);
// RF24Network network(radio);

// GenericClient<VescData, ControllerData> controllerClient(COMMS_CONTROLLER);

// void send_to_vesc(uint8_t throttle, bool cruise_control);

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
#ifdef DEBUG_SERIAL
  Serial.begin(115200);
#endif

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

  createLocalQueueManagers();

  configureTasks();

  startTasks();

  waitForTasks();

  enableTasks();

  pinMode(26, OUTPUT);
}

Queue1::Manager<SimplMessageObj> *simplMsgQueue = nullptr;

elapsedMillis sinceCheckedCtrlOnline, sinceToggleLight, sinceCheckedQueues;

void loop()
{
  if (sinceCheckedQueues > 500)
  {
    sinceCheckedQueues = 0;
    if (simplMsgQueue->hasValue())
    {
      if (simplMsgQueue->payload.message == SIMPL_HEADLIGHT_ON)
      {
        digitalWrite(26, HIGH);
      }
      else if (simplMsgQueue->payload.message == SIMPL_HEADLIGHT_OFF)
      {
        digitalWrite(26, LOW);
      }
    }
  }

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
  simplMsgQueue = createQueue<SimplMessageObj>("(loop) simplMsgQueue");
}

void configureTasks()
{
  ctrlrCommsTask.doWorkInterval = PERIOD_10ms;
  ctrlrCommsTask.printRxFromController = true;

  footLightTask.doWorkInterval = PERIOD_100ms;
  footLightTask.printStateChange = true;

#ifdef USING_M5STACK_DISPLAY
  m5StackDisplayTask.doWorkInterval = PERIOD_100ms;
#endif
#if SEND_TO_VESC == 0
  mockVescTask.doWorkInterval = PERIOD_50ms;
#endif

  vescCommsTask.doWorkInterval = PERIOD_10ms;
}

#define USE_M5STACK_DISPLAY 0
#define USE_M5STACK_BUTTONS 1

void startTasks()
{
  ctrlrCommsTask.start(nsControllerCommsTask::task1);
  footLightTask.start(nsFootlightTask::task1);
  vescCommsTask.start(nsVescCommsTask::task1);
#ifdef USING_M5STACK_DISPLAY
  m5StackDisplayTask.start(nsM5StackDisplayTask::task1);
#endif
#if SEND_TO_VESC == 0
  mockVescTask.start(nsMockVescTask::task1);
#endif
}

elapsedMillis since_reported_waiting;

void waitForTasks()
{
  while (
      !ctrlrCommsTask.ready ||
      !footLightTask.ready ||
      !vescCommsTask.ready ||
#ifdef USING_M5STACK_DISPLAY
      !m5StackDisplayTask.ready ||
#endif
#if SEND_TO_VESC == 0
      !mockVescTask.ready ||
#endif
      false)
    vTaskDelay(PERIOD_10ms);
  Serial.printf("-- all tasks ready! --\n");
}

void enableTasks(bool print)
{
  ctrlrCommsTask.enable(print);
  footLightTask.enable(print);
  vescCommsTask.enable(print);
#ifdef USING_M5STACK_DISPLAY
  m5StackDisplayTask.enable(print);
#endif
#if SEND_TO_VESC == 0
  mockVescTask.enable(print);
#endif
}
