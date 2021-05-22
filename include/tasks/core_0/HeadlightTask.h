#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <VescData.h>

#define HEADLIGHT_TASK
#define LIGHTS_OFF 0
#define LIGHTS_ON 1

namespace nsHeadlightTask
{

  // prototypes
  void _handleSimplMessage(SimplMessageObj obj);
  void addTransitions();
  const char *getEvent(uint16_t ev);
  const char *getState(uint16_t st);

  void stateOff_OnEnter();
  void stateOff_OnLoop();
  void stateOn_OnEnter();
  void stateActive_OnEnter();
  void stateActive_OnLoop();
  void stateInactive_OnEnter();
  void stateInactive_Loop();
  void stateInactive_OnExit();

  State stateOff(stateOff_OnEnter, stateOff_OnLoop, NULL);
  State stateOn(stateOn_OnEnter, NULL, NULL);
  State stateInactive(stateInactive_OnEnter, stateInactive_Loop, stateInactive_OnExit);
  State stateActive(stateActive_OnEnter, stateActive_OnLoop, NULL);

  enum StateID
  {
    ST_OFF = 0,
    ST_ON,
    ST_ACTIVE,
    ST_INACTIVE,
  };

  enum Event
  {
    OFF = 0,
    TOGGLE,
    MOVING,
    STOPPED,
    INACTIVE,
  };

  //----------------------------------------

  Fsm *fsm1;

  FsmManager<Event> fsm_mgr;
}

class HeadlightTask : public TaskBase
{

public:
  bool printWarnings = true, printState = false;

private:
  Queue1::Manager<VescData> *vescDataQueue = nullptr;
  Queue1::Manager<SimplMessageObj> *simplMsgQueue = nullptr;

  SimplMessageObj _simplMsg;

  uint8_t lightState = LIGHTS_OFF;

  VescData vescData;

public:
  HeadlightTask() : TaskBase("HeadlightTask", /*stacksize*/ 10000)
  {
    _core = CORE_0;
  }
  //------------------------------
  void turnLights(bool on)
  {
    _simplMsg.message = on
                            ? SIMPL_HEADLIGHT_ON
                            : SIMPL_HEADLIGHT_OFF;
    simplMsgQueue->send(&_simplMsg);
  }
  //------------------------------
  void flashLights()
  {
    _simplMsg.message = SIMPL_HEADLIGHT_FLASH;
    simplMsgQueue->send(&_simplMsg);
  }

private:
  //------------------------------
  void _initialise()
  {
    vescDataQueue = createQueue<VescData>("(HeadlightTask) vescDataQueue");
    vescDataQueue->printMissedPacket = false; // happens a lot since slow doWorkInterval
    simplMsgQueue = createQueue<SimplMessageObj>("(HeadlightTask) simplMsgQueue");
    simplMsgQueue->printMissedPacket = true;

    using namespace nsHeadlightTask;

    fsm1 = new Fsm(&stateOff);
    fsm_mgr.begin(fsm1);
    fsm_mgr.setPrintStateCallback(
        [](uint16_t st)
        {
          Serial.printf("HeadlightTask state: %s\n", getState(st));
        });
    fsm_mgr.setPrintTriggerCallback(
        [](uint16_t trigger)
        {
          Serial.printf("HeadlightTask trigger: %s\n", getEvent(trigger));
        });

    addTransitions();
  }
  //------------------------------
  void doWork()
  {
    if (vescDataQueue->hasValue())
      _handleVescData(vescDataQueue->payload);

    if (simplMsgQueue->hasValue())
      _handleSimplMessage(simplMsgQueue->payload);

    nsHeadlightTask::fsm_mgr.runMachine();
  }
  //------------------------------
  void cleanup()
  {
    delete (vescDataQueue);
    delete (simplMsgQueue);
  }

private:
  void _handleVescData(VescData &p_vescData)
  {
    using namespace nsHeadlightTask;
    bool stopped = vescData.moving && !p_vescData.moving;
    bool moving = !vescData.moving && p_vescData.moving;
    vescData = p_vescData;

    if (moving)
    {
      this->setRunningState(RunningState::SLOW);
      fsm_mgr.trigger(Event::MOVING);
    }
    else if (stopped)
    {
      this->setRunningState(RunningState::FAST);
      fsm_mgr.trigger(Event::STOPPED);
    }
  }
  //------------------------------
  void _handleSimplMessage(SimplMessageObj obj)
  {
    using namespace nsHeadlightTask;

    _simplMsg = obj;

    if (_simplMsg.message == I2C_INPUT_7_PRESSED)
    {
      fsm_mgr.trigger(Event::TOGGLE);
    }
  }
  //------------------------------
};
//========================================================================
HeadlightTask headlightTask;

namespace nsHeadlightTask
{
  void task1(void *parameters)
  {
    headlightTask.task(parameters);
  }

  elapsedMillis sinceStopped = 0, sinceFlashedInactive = 0;

  void stateOff_OnEnter()
  {
    DEBUG("HeadlightTask: stateOff_OnEnter");
    sinceStopped = 0;
    headlightTask.turnLights(LIGHTS_OFF);
  }

  void stateOff_OnLoop()
  {
    if (FEATURE_INACTIVITY_FLASH &&
        sinceStopped > FEATURE_INACTIVITY_TIMEOUT_IN_SECONDS * SECONDS)
      fsm_mgr.trigger(Event::INACTIVE);
  }

  void stateOn_OnEnter()
  {
    DEBUG("HeadlightTask: stateOn_OnEnter");
    headlightTask.turnLights(LIGHTS_ON);
  }

  void stateActive_OnEnter()
  {
    DEBUG("HeadlightTask: stateActive_OnEnter");
    sinceStopped = 0;
  }

  void stateActive_OnLoop()
  {
  }

  void stateInactive_OnEnter()
  {
    DEBUG("HeadlightTask: stateInactive_OnEnter");
    headlightTask.turnLights(LIGHTS_OFF);
  }
  void stateInactive_Loop()
  {
    if (sinceFlashedInactive > FEATURE_INACTIVITY_FLASH_INTERVAL_IN_SECONDS * SECONDS)
    {
      sinceFlashedInactive = 0;
      headlightTask.flashLights();
    }
  }
  void stateInactive_OnExit()
  {
    headlightTask.turnLights(LIGHTS_OFF);
  }

  void addTransitions()
  {
    fsm1->add_transition(&stateOff, &stateOn, Event::TOGGLE, NULL);
    fsm1->add_transition(&stateOn, &stateOff, Event::TOGGLE, NULL);

    if (FEATURE_INACTIVITY_FLASH)
    {
      fsm1->add_transition(&stateOff, &stateActive, Event::MOVING, NULL);
      fsm1->add_transition(&stateOff, &stateInactive, Event::INACTIVE, NULL);
      fsm1->add_transition(&stateInactive, &stateActive, Event::MOVING, NULL);
      fsm1->add_transition(&stateInactive, &stateOn, Event::TOGGLE, NULL);
      fsm1->add_transition(&stateActive, &stateActive, Event::MOVING, NULL);
      fsm1->add_transition(&stateActive, &stateOff, Event::STOPPED, NULL);
    }
  }

  const char *getEvent(uint16_t ev)
  {
    switch (ev)
    {
    case OFF:
      return "OFF";
    case TOGGLE:
      return "TOGGLE";
    case MOVING:
      return "MOVING";
    case STOPPED:
      return "STOPPED";
    case INACTIVE:
      return "INACTIVE";
    }
    return "OUT OF RANGE (HeadlightTask getEvent())";
  }

  const char *getState(uint16_t st)
  {
    switch (st)
    {
    case ST_OFF:
      return "ST_OFF";
    case ST_ON:
      return "ST_ON";
    case ST_ACTIVE:
      return "ST_ACTIVE";
    case ST_INACTIVE:
      return "ST_INACTIVE";
    }
    return "OUT OF RANGE (HeadlightTask getState())";
  }

}
