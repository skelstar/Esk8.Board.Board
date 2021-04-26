#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <tasks/core_0/Helpers/M5StackDisplayTaskFsm.h>

namespace m5Stack = nsM5StackDisplayTask;

class M5StackDisplayTask : public TaskBase
{
public:
public:
  M5StackDisplayTask(unsigned long p_doWorkInterval)
      : TaskBase("M5StackDisplayTask", 3000, p_doWorkInterval)
  {
  }

private:
private:
  Queue1::Manager<ControllerData> *controllerQueue = nullptr;
  Queue1::Manager<VescData> *vescQueue = nullptr;

  void initialiseQueues()
  {
    controllerQueue = createQueue<ControllerData>("(M5StackDisplayTask) controllerQueue");
    vescQueue = createQueue<VescData>("(M5StackDisplayTask) vescQueue");
  }

  void initialise()
  {
    m5Stack::initTFT();

    m5Stack::initFsm();
  }

  void initialTask()
  {
    // doWork();
  }

  bool timeToDoWork()
  {
    return true;
  }

  void doWork()
  {
    if (controllerQueue->hasValue())
    {
      ControllerData::print(controllerQueue->payload, "[M5StackDisplay]");

      if (controller.throttleChanged())
      {
        if (controller.data.throttle == 127 && !m5Stack::fsm_mgr.currentStateIs(m5Stack::ST_STOPPED))
          m5Stack::fsm_mgr.trigger(m5Stack::TR_STOPPED);
        else if (controller.data.throttle > 127)
          m5Stack::fsm_mgr.trigger(m5Stack::TR_MOVING);
        else if (controller.data.throttle < 127)
          m5Stack::fsm_mgr.trigger(m5Stack::TR_BRAKING);
      }
    }

    if (vescQueue->hasValue())
    {
      if (vescQueue->payload.moving && m5Stack::fsm_mgr.currentStateIs(m5Stack::ST_MOVING) == false)
        m5Stack::fsm_mgr.trigger(m5Stack::TR_MOVING);
      else if (vescQueue->payload.moving == false && m5Stack::fsm_mgr.currentStateIs(m5Stack::ST_STOPPED) == false)
        m5Stack::fsm_mgr.trigger(m5Stack::TR_STOPPED);
    }

    m5Stack::fsm_mgr.runMachine();
  }
};

M5StackDisplayTask m5StackDisplayTask(PERIOD_50ms);

namespace nsM5StackDisplayTask
{
  void task1(void *parameters)
  {
    m5StackDisplayTask.task(parameters);
  }
}

void m5Stack::stateReadyOnEnter()
{
  m5Stack::fsm_mgr.printState(m5Stack::ST_READY);
}

void m5Stack::stateMovingOnEnter()
{
  m5Stack::fsm_mgr.printState(m5Stack::ST_MOVING);
  // TODO read from local instead
  uint8_t t = controller.data.throttle;
  char thr[10];
  sprintf(thr, "%03d", t);
  m5Stack::drawCard(thr, TFT_WHITE, TFT_DARKGREEN);
}

void m5Stack::stateStoppedOnEnter()
{
  m5Stack::fsm_mgr.printState(m5Stack::ST_STOPPED);
  m5Stack::drawCard("STOPPED", TFT_WHITE, TFT_BLACK);
}

void m5Stack::stateBrakingOnEnter()
{
  m5Stack::fsm_mgr.printState(m5Stack::ST_BRAKING);
  uint8_t t = controller.data.throttle;
  char thr[10];
  sprintf(thr, "%03d", t);
  m5Stack::drawCard(thr, TFT_WHITE, TFT_RED);
}
