#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <VescData.h>

namespace nsHeadlightTask
{
  // prototypes
  void _handleSimplMessage(SimplMessageObj obj);
}

class HeadlightTask : public TaskBase
{
public:
  bool printWarnings = true;

private:
  Queue1::Manager<VescData> *vescDataQueue = nullptr;
  Queue1::Manager<SimplMessageObj> *simplMsgQueue = nullptr;

  const uint8_t LIGHTS_OFF = 0;
  const uint8_t LIGHTS_ON = 1;

  uint8_t lightState = LIGHTS_OFF;

  VescData vescData;

public:
  HeadlightTask() : TaskBase("HeadlightTask", 5000, PERIOD_50ms)
  {
    _core = CORE_0;
    _priority = TASK_PRIORITY_3;
  }

private:
  void initialise()
  {
    vescDataQueue = createQueue<VescData>("(HeadlightTask) vescDataQueue");
    simplMsgQueue = createQueue<SimplMessageObj>("(HeadlightTask) simplMsgQueue");
  }

  void doWork()
  {
    if (vescDataQueue->hasValue())
    {
      vescData.moving = vescDataQueue->payload.moving;
      _turnLights(vescData.moving == true);
    }

    if (simplMsgQueue->hasValue())
      _handleSimplMessage(simplMsgQueue->payload);
  } // doWork

  void cleanup()
  {
    delete (vescDataQueue);
    delete (simplMsgQueue);
  }

private:
  void _handleSimplMessage(SimplMessageObj obj)
  {
  }

  void _turnLights(bool on)
  {
    if (lightState == LIGHTS_OFF && on)
    {
      digitalWrite(HEADLIGHT_PIN, LIGHTS_ON);
      lightState = LIGHTS_ON;
    }
    else if (lightState == LIGHTS_ON && !on)
    {
      digitalWrite(HEADLIGHT_PIN, LIGHTS_OFF);
      lightState = LIGHTS_OFF;
    }
  }
};

HeadlightTask headlightTask;

namespace nsHeadlightTask
{
  void task1(void *parameters)
  {
    headlightTask.task(parameters);
  }
}
