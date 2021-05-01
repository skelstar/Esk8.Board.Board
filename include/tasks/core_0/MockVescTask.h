#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <VescData.h>
#include <Button2.h>

const uint8_t M5_BUTTON_A = 39;
const uint8_t M5_BUTTON_B = 38;
const uint8_t M5_BUTTON_C = 37;

Button2 buttonA(M5_BUTTON_A);
Button2 buttonB(M5_BUTTON_B);
Button2 buttonC(M5_BUTTON_C);

bool buttonsChanged;
bool wasMoving = false;

// prototypes
void buttonAPressed(Button2 &btn);
void buttonAReleased(Button2 &btn);
void buttonCReleased(Button2 &btn);

//============================================

class MockVescTask : public TaskBase
{
public:
  bool printWarnings = true;
  bool mockMovingLoopMode = false;
  int mockMovingLoopState = 0;

  VescData *vescData;

private:
  Queue1::Manager<VescData> *vescDataQueue = nullptr;
  Queue1::Manager<SimplMessageObj> *simplMessageQueue = nullptr;

  SimplMessageObj _simplMessage;

public:
  MockVescTask() : TaskBase("MockVescTask", 3000, PERIOD_50ms)
  {
    _core = CORE_0;
    _priority = TASK_PRIORITY_0;
  }

  void sendSimplMessage(SimplMessage message)
  {
    _simplMessage.message = message;
    simplMessageQueue->send(&_simplMessage);
  }

private:
  void initialiseQueues()
  {
    vescDataQueue = createQueue<VescData>("(MockVescTask) vescDataQueue");
    simplMessageQueue = createQueue<SimplMessageObj>("(MockVescTask) simplMessageQueue");

    simplMessageQueue->read(); // clear the queue
  }

  void initialise()
  {
    vescData = new VescData();
    buttonsChanged = false;
    m5StackButtonsInit();
  }

  bool timeToDoWork()
  {
    return true;
  }

  void doWork()
  {
    if (vescDataQueue->hasValue())
    {
      float oldBatt = vescData->batteryVoltage;
      vescData = new VescData(vescDataQueue->payload);
      vescData->moving = buttonA.isPressed();
      vescData->batteryVoltage = oldBatt;
      // VescData::print(*vescData, "[MockVescTask] vescDataQueue:read");
    }

    if (simplMessageQueue->hasValue())
    {
      SimplMessageObj::print(simplMessageQueue->payload, "-->[MockVescTask]");
    }

    buttonA.loop();
    buttonB.loop();
    buttonC.loop();

    if (buttonsChanged)
    {
      vescDataQueue->send(vescData);
      buttonsChanged = false;
    }
  } // doWork

  void cleanup()
  {
    delete (vescDataQueue);
  }
  //------------------------------------------

  void m5StackButtonsInit()
  {
    // ButtonA
    buttonA.setPressedHandler(buttonAPressed);
    buttonA.setReleasedHandler(buttonAReleased);

    // ButtonB
    buttonB.setPressedHandler([](Button2 &btn) {
    });
    buttonB.setReleasedHandler([](Button2 &btn) {
    });

    buttonC.setReleasedHandler(buttonCReleased);
  }
};  // namespace Buttons
    //=====================================================

static MockVescTask mockVescTask;

namespace nsMockVescTask
{
  void task1(void *parameters)
  {
    mockVescTask.task(parameters);
  }
}

static VescData mockMoving(VescData v, bool buttonHeld)
{
  v.moving = buttonHeld;
  if (buttonHeld)
  {
    if (v.batteryVoltage <= 10.0)
      v.batteryVoltage = 43.3;
    v.motorCurrent = 3;
    v.ampHours += v.motorCurrent;
  }
  else if (wasMoving && !buttonHeld)
  {
    v.moving = false;
  }
  wasMoving = buttonHeld;
  return v;
}

void buttonAPressed(Button2 &btn)
{
  Serial.printf("buttonA pressed\n");
  *mockVescTask.vescData = mockMoving(*(mockVescTask.vescData), true);
  buttonsChanged = true;
}

void buttonAReleased(Button2 &btn)
{
  *mockVescTask.vescData = mockMoving(*(mockVescTask.vescData), false);
  buttonsChanged = true;
}

void buttonCReleased(Button2 &btn)
{
  mockVescTask.sendSimplMessage(SIMPL_MOCK_MOVING_LOOP);
}
