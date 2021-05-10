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
  Queue1::Manager<SimplMessageObj> *simplMsgQueue = nullptr;

  void _initialise()
  {
    controllerQueue = createQueue<ControllerData>("(M5StackDisplayTask) controllerQueue");
    vescQueue = createQueue<VescData>("(M5StackDisplayTask) vescQueue");
    simplMsgQueue = createQueue<SimplMessageObj>("(M5StackDisplayTask) simplMsgQueue");

    m5Stack::initTFT();

    m5Stack::initFsm();
  }

  void doWork()
  {
    using namespace m5Stack;

    if (controllerQueue->hasValue())
    {
      if (controller.throttleChanged())
      {
        if (controller.data.throttle == 127 && NOT_IN_STATE(ST_STOPPED))
          TRIGGER(TR_STOPPED);
        else if (controller.data.throttle > 127)
          TRIGGER(TR_MOVING);
        else if (controller.data.throttle < 127)
          TRIGGER(TR_BRAKING);
      }
    }

    if (simplMsgQueue->hasValue())
    {
      if (simplMsgQueue->payload.message == SIMPL_HEADLIGHT_ON ||
          simplMsgQueue->payload.message == SIMPL_HEADLIGHT_OFF)
      {
        _g_HeadlightState = simplMsgQueue->payload.message == SIMPL_HEADLIGHT_ON;
        TRIGGER(simplMsgQueue->payload.message == SIMPL_HEADLIGHT_ON
                    ? TR_HEADLIGHT_ON
                    : TR_HEADLIGHT_OFF);
      }
    }

    if (vescQueue->hasValue())
    {
      if (vescQueue->payload.moving && NOT_IN_STATE(ST_MOVING))
        TRIGGER(TR_MOVING);
      else if (vescQueue->payload.moving == false && NOT_IN_STATE(ST_STOPPED))
        TRIGGER(TR_STOPPED);
    }

    fsm_mgr.runMachine();
  }

  void
  cleanup()
  {
    delete (controllerQueue);
    delete (vescQueue);
  }
};

M5StackDisplayTask m5StackDisplayTask(PERIOD_50ms);

namespace nsM5StackDisplayTask
{
  void task1(void *parameters)
  {
    m5StackDisplayTask.task(parameters);
  }

  void stateReadyOnEnter()
  {
    PRINT_STATE(ST_READY);
  }

  void stateMovingOnEnter()
  {
    PRINT_STATE(ST_MOVING);
    // TODO read from local instead
    uint8_t t = controller.data.throttle;
    char thr[10];
    sprintf(thr, "%03d", t);
    drawCard(thr, TFT_WHITE, TFT_DARKGREEN);
  }

  void stateStoppedOnEnter()
  {
    PRINT_STATE(ST_STOPPED);
    drawCard(_g_HeadlightState == true ? "HEADLIGHT" : "STOPPED", TFT_WHITE, TFT_BLACK);
  }

  void stateBrakingOnEnter()
  {
    PRINT_STATE(ST_BRAKING);
    uint8_t t = controller.data.throttle;
    char thr[10];
    sprintf(thr, "%03d", t);
    drawCard(thr, TFT_WHITE, TFT_RED);
  }
}
