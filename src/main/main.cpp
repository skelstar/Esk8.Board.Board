#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

SemaphoreHandle_t mux_I2C = nullptr;
SemaphoreHandle_t mux_SPI = nullptr;

#include <Wire.h>

#include <shared-utils.h>
#include <types.h>
#include <printFormatStrings.h>
#include <utils.h>
#include <FsmManager.h>
#include <SharedConstants.h>
#include <QueueManager.h>
#include <constants.h>
#include <macros.h>
#include <tasks/queues/types/ControllerClass.h>
#include <tasks/queues/types/I2CPinsType.h>

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI(LCD_HEIGHT, LCD_WIDTH);

ControllerClass controller;

#include <tasks/queues/queues.h>

#include <RF24Network.h>
#include <NRF24L01Lib.h>
#include <GenericClient.h>

#include <QueueManager.h>

#include <tasks/root.h>

int tasksCount = 0;

TaskBase *tasks[NUM_TASKS];

#include <Fsm.h>

elapsedMillis
    sinceLastControllerPacket,
    sinceBoardBooted,
    sinceUpdatedButtonAValues,
    sinceNRFUpdate;

bool controller_connected = false;

//------------------------------------------------------------------

#define NUM_RETRIES 5

//------------------------------------------------------------------

void populateTaskList();
void createQueues();
void createLocalQueueManagers();
void startTasks();
void initialiseTasks();
void configureTasks();
void waitForTasks();
void enableTasks(bool print = false);

//============================================================

void setup()
{
#ifdef DEBUG_SERIAL
  Serial.begin(115200);
#endif

#ifdef I2CPORTEXP1_TASK
  Wire.begin();
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

  populateTaskList();

  createLocalQueueManagers();

  configureTasks();

  startTasks();

  initialiseTasks();

  waitForTasks();

  enableTasks();

  pinMode(26, OUTPUT);
}

Queue1::Manager<SimplMessageObj> *simplMsgQueue = nullptr;

elapsedMillis sinceCheckedCtrlOnline, sinceToggleLight, sinceCheckedQueues;

void loop()
{
  vTaskDelay(10);
}

//===================================================

void addTaskToList(TaskBase *t)
{
  Serial.printf("Adding task %s\n", t->_name);
  if (tasksCount < NUM_TASKS)
  {
    tasks[tasksCount++] = t;
    assert(tasksCount < NUM_TASKS);
  }
}

void populateTaskList()
{
  addTaskToList(&ctrlrCommsTask);
  addTaskToList(&headlightTask);
  addTaskToList(&i2cPortExpTask);
  addTaskToList(&vescCommsTask);

#ifdef FOOTLIGHT_TASK
  addTaskToList(&footLightTask);
#endif
#ifdef I2COLED_TASK
  addTaskToList(&i2cOledTask);
#endif
#ifdef IMU_TASK
  addTaskToList(&imuTask);
#endif
#ifdef M5STACKDISPLAY_TASK
  addTaskToList(&m5StackDisplayTask);
#endif
#if USING_M5STACK == 1 && SEND_TO_VESC == 0
  addTaskToList(&mockVescTask);
#endif
}

void createQueues()
{
  xControllerQueueHandle = xQueueCreate(1, sizeof(ControllerClass *));
  xVescQueueHandle = xQueueCreate(1, sizeof(VescData *));
  xI2CPinsQueue = xQueueCreate(1, sizeof(I2CPinsType *));
}

#include <tasks/queues/QueueFactory.h>

void createLocalQueueManagers()
{
  simplMsgQueue = createQueue<SimplMessageObj>("(loop) simplMsgQueue");
}

void configureTasks()
{
  DEBUG("Configuring tasks");

  ctrlrCommsTask.doWorkIntervalFast = PERIOD_10ms;
  ctrlrCommsTask.priority = TASK_PRIORITY_4;
  // ctrlrCommsTask.printRxFromController = true;

#ifdef FOOTLIGHT_TASK
  footLightTask.doWorkIntervalFast = PERIOD_100ms;
  footLightTask.priority = TASK_PRIORITY_0;
  // footLightTask.printStateChange = true;
#endif

  headlightTask.doWorkIntervalSlow = PERIOD_200ms;
  headlightTask.doWorkIntervalFast = PERIOD_50ms;
  headlightTask.priority = TASK_PRIORITY_1; // TODO disable when moving?

#ifdef I2COLED_TASK
  i2cOledTask.doWorkIntervalFast = PERIOD_100ms;
  i2cOledTask.priority = TASK_PRIORITY_2;
#endif

  i2cPortExpTask.doWorkIntervalFast = PERIOD_100ms;
  i2cPortExpTask.priority = TASK_PRIORITY_0;

#ifdef IMU_TASK
  imuTask.doWorkIntervalFast = PERIOD_200ms;
  imuTask.priority = TASK_PRIORITY_0;
#endif

#ifdef M5STACKDISPLAY_TASK
  m5StackDisplayTask.doWorkIntervalFast = PERIOD_100ms;
  m5StackDisplayTask.priority = TASK_PRIORITY_2;
#endif
#if USING_M5STACK == 1 && SEND_TO_VESC == 0
  mockVescTask.doWorkIntervalFast = PERIOD_50ms;
  mockVescTask.priority = TASK_PRIORITY_0;
#endif

  vescCommsTask.doWorkIntervalFast = PERIOD_20ms;
  vescCommsTask.priority = TASK_PRIORITY_3;
  // vescCommsTask.printReadFromVesc = true;
  // vescCommsTask.printSentToVesc = true;
}

#define USE_M5STACK_DISPLAY 0
#define USE_M5STACK_BUTTONS 1

void startTasks()
{
  DEBUG("Starting tasks");

  ctrlrCommsTask.start(nsControllerCommsTask::task1);
  headlightTask.start(nsHeadlightTask::task1);
  i2cPortExpTask.start(nsI2CPortExp1Task::task1);
  vescCommsTask.start(nsVescCommsTask::task1);

#ifdef FOOTLIGHT_TASK
  footLightTask.start(nsFootlightTask::task1);
#endif
#ifdef I2COLED_TASK
  i2cOledTask.start(nsI2COledTask::task1);
#endif
#ifdef IMU_TASK
  imuTask.start(nsIMUTask::task1);
#endif
#ifdef M5STACKDISPLAY_TASK
  m5StackDisplayTask.start(nsM5StackDisplayTask::task1);
#endif
#if MOCK_VESC == 1 && SEND_TO_VESC == 0
  mockVescTask.start(nsMockVescTask::task1);
#endif
}

void initialiseTasks()
{
  DEBUG("Initialising tasks");

  for (int i = 0; i < tasksCount; i++)
    tasks[i]->initialiseTask(PRINT_THIS);
}

void waitForTasks()
{
  bool allReady = false;
  while (!allReady)
  {
    allReady = true;
    for (int i = 0; i < tasksCount; i++)
      allReady = allReady && tasks[i]->ready;
    vTaskDelay(PERIOD_100ms);
    DEBUG("Waiting for tasks\n");
  }
  DEBUG("-- all tasks ready! --");
}

void enableTasks(bool print)
{
  for (int i = 0; i < tasksCount; i++)
    tasks[i]->enable(print);
}
