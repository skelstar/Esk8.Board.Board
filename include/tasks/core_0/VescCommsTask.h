#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <VescData.h>

class VescCommsTask : public TaskBase
{
public:
  bool printWarnings = true;

private:
  // BatteryInfo Prototype;

  Queue1::Manager<ControllerData> *controllerQueue = nullptr;
  Queue1::Manager<VescData> *vescDataQueue = nullptr;

public:
  VescCommsTask(unsigned long p_doWorkInterval) : TaskBase("VescCommsTask", 5000, p_doWorkInterval)
  {
    _core = CORE_1;
    _priority = TASK_PRIORITY_3;
  }

private:
  void initialiseQueues()
  {
    controllerQueue = createQueue<ControllerData>("(VescCommsTask) controllerQueue");
    controllerQueue->read(); // clear the queue
    vescDataQueue = createQueue<VescData>("(VescCommsTask) vescDataQueue");
  }

  void initialise()
  {
  }

  bool timeToDoWork()
  {
    return true;
  }

  void doWork()
  {
    if (controllerQueue->hasValue())
    {
      ControllerData::print(controllerQueue->payload, "[VescCommsTask]");
    }
  }
};

VescCommsTask vescCommsTask(PERIOD_10ms);

namespace nsVescCommsTask
{
  void task1(void *parameters)
  {
    vescCommsTask.task(parameters);
  }
}
