#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <ControllerClass.h>
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
  VescCommsTask(unsigned long p_doWorkInterval) : TaskBase("VescCommsTask", 3000, p_doWorkInterval)
  {
    _core = CORE_1;
    _priority = TASK_PRIORITY_3;
  }

private:
  void initialiseQueues()
  {
    controllerQueue = createQueue<ControllerData>("(VescCommsTask) controllerQueue");
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
  }
};

VescCommsTask vescCommsTask(PERIOD_50ms);

namespace nsVescCommsTask
{
  void task1(void *parameters)
  {
    vescCommsTask.task(parameters);
  }
}
