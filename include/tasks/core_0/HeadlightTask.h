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
  bool printWarnings = true, printState = false;

private:
  Queue1::Manager<VescData> *vescDataQueue = nullptr;
  Queue1::Manager<SimplMessageObj> *simplMsgQueue = nullptr;

  SimplMessageObj _simplMsg;

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
  void _initialise()
  {
    vescDataQueue = createQueue<VescData>("(HeadlightTask) vescDataQueue");
    simplMsgQueue = createQueue<SimplMessageObj>("(HeadlightTask) simplMsgQueue");
  }

  void doWork()
  {
    if (vescDataQueue->hasValue())
      _handleVescData(vescDataQueue->payload);

    if (simplMsgQueue->hasValue())
      _handleSimplMessage(simplMsgQueue->payload);
  }

  void cleanup()
  {
    delete (vescDataQueue);
    delete (simplMsgQueue);
  }

private:
  void _handleVescData(VescData &p_vescData)
  {
    vescData = p_vescData;
  }

  void _handleSimplMessage(SimplMessageObj obj)
  {
    _simplMsg = obj;

    if (_simplMsg.message == I2C_INPUT_7_PRESSED)
    {
      // toggle lights
      lightState = lightState == LIGHTS_ON
                       ? LIGHTS_OFF
                       : LIGHTS_ON;

      _turnLights(lightState == LIGHTS_ON);

      if (printState)
        Serial.printf("HEADLIGHT light: %s\n",
                      lightState == LIGHTS_ON ? "LIGHTS_ON" : "LIGHTS_OFF");
    }
  }

  void _turnLights(bool on)
  {
    _simplMsg.message = on
                            ? SIMPL_HEADLIGHT_ON
                            : SIMPL_HEADLIGHT_OFF;
    simplMsgQueue->send(&_simplMsg);
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
