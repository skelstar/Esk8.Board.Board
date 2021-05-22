#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <VescData.h>
#include <Button2.h>

#define MOCKVESC_TASK

const uint8_t M5_BUTTON_A = 39;
const uint8_t M5_BUTTON_B = 38;
const uint8_t M5_BUTTON_C = 37;

Button2 buttonA(M5_BUTTON_A);
Button2 buttonB(M5_BUTTON_B);
Button2 buttonC(M5_BUTTON_C);

bool buttonsChanged;
bool wasMoving = false;
bool headLightState = false;

// prototypes
void buttonAPressed(Button2 &btn);
void buttonAReleased(Button2 &btn);
void buttonBPressed(Button2 &btn);
void buttonBReleased(Button2 &btn);
void buttonCReleased(Button2 &btn);

//============================================

class MockVescTask : public TaskBase
{
public:
  bool printWarnings = true;
  bool mockMovingLoopMode = false;
  int mockMovingLoopState = 0;

  VescData vescData;

private:
  Queue1::Manager<VescData> *vescDataQueue = nullptr;
  Queue1::Manager<SimplMessageObj> *simplMessageQueue = nullptr;

  SimplMessageObj _simplMessage;

public:
  MockVescTask() : TaskBase("MockVescTask", 5000, PERIOD_50ms)
  {
    _core = CORE_0;
  }

  void sendSimplMessage(SimplMessage message)
  {
    _simplMessage.message = message;
    simplMessageQueue->send(&_simplMessage);
  }

private:
  void _initialise()
  {
    vescDataQueue = createQueue<VescData>("vescDataQueue");
    vescDataQueue->printMissedPacket = false;

    simplMessageQueue = createQueue<SimplMessageObj>("simplMessageQueue");
    simplMessageQueue->read(); // clear the queue

    buttonsChanged = false;
    headLightState = false;
    m5StackButtonsInit();
  }

  void doWork()
  {
    if (vescDataQueue->hasValue())
    {
      float oldBatt = vescData.batteryVoltage;
      vescData = vescDataQueue->payload;
      vescData.moving = buttonA.isPressed();
      vescData.batteryVoltage = oldBatt;
    }

    if (simplMessageQueue->hasValue())
    {
      simplMessageQueue->payload.print("-->[MockVescTask]");
    }

    buttonA.loop();
    buttonB.loop();
    buttonC.loop();

    if (buttonsChanged)
    {
      vescDataQueue->send(&vescData);
      buttonsChanged = false;
    }
  } // doWork

  void cleanup()
  {
    delete (vescDataQueue);
    delete (simplMessageQueue);
  }
  //------------------------------------------

  void m5StackButtonsInit()
  {
    // ButtonA
    buttonA.setPressedHandler(buttonAPressed);
    buttonA.setReleasedHandler(buttonAReleased);

    // ButtonB
    buttonB.setPressedHandler(buttonBPressed);
    buttonB.setReleasedHandler(buttonBReleased);

    // Button C
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

static void mockMoving(VescData &v, bool buttonHeld)
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
}

void buttonAPressed(Button2 &btn)
{
  Serial.printf("buttonA pressed\n");
  mockMoving(mockVescTask.vescData, true);
  buttonsChanged = true;
}

void buttonAReleased(Button2 &btn)
{
  mockMoving(mockVescTask.vescData, /*button held*/ false);
  buttonsChanged = true;
}

void buttonBPressed(Button2 &B)
{
  mockVescTask.sendSimplMessage(SIMPL_HEADLIGHT_ON);
}

void buttonBReleased(Button2 &B)
{
  mockVescTask.sendSimplMessage(SIMPL_HEADLIGHT_OFF);
}

void buttonCReleased(Button2 &btn)
{
  mockVescTask.sendSimplMessage(SIMPL_TOGGLE_MOCK_MOVING_LOOP);
}
