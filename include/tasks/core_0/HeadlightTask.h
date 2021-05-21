#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <VescData.h>

#define HEADLIGHT_TASK

namespace nsHeadlightTask
{
  // prototypes
  void _handleSimplMessage(SimplMessageObj obj);
  void addTransitions();
  const char *getState(uint16_t st);

  void stateOff_OnEnter();
  void stateOn_OnEnter();
  void stateActive_OnEnter();
  void stateInactive_OnEnter();

  State stateOff(stateOff_OnEnter, NULL, NULL);
  State stateOn(stateOn_OnEnter, NULL, NULL);
  State stateActive(stateActive_OnEnter, NULL, NULL);
  State stateInactive(stateInactive_OnEnter, NULL, NULL);

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

  const uint8_t LIGHTS_OFF = 0;
  const uint8_t LIGHTS_ON = 1;

  uint8_t lightState = LIGHTS_OFF;

  VescData vescData;

public:
  HeadlightTask() : TaskBase("HeadlightTask", 5000, PERIOD_50ms)
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

private:
  //------------------------------
  void _initialise()
  {
    vescDataQueue = createQueue<VescData>("(HeadlightTask) vescDataQueue");
    simplMsgQueue = createQueue<SimplMessageObj>("(HeadlightTask) simplMsgQueue");
    simplMsgQueue->printMissedPacket = true;

    using namespace nsHeadlightTask;

    fsm1 = new Fsm(&stateOff);
    fsm_mgr.begin(fsm1);
    // fsm_mgr.setPrintTriggerCallback(
    //     [](uint16_t trigger)
    //     {
    //       DEBUGMVAL("HeadlightTask trigger: %s\n", getState(trigger));
    //     });

    addTransitions();
  }
  //------------------------------
  elapsedMillis sinceLastFlashed;
  const unsigned long inactivityLightsFlashInterval = 10000;

  void doWork()
  {
    if (vescDataQueue->hasValue())
      _handleVescData(vescDataQueue->payload);

    if (simplMsgQueue->hasValue())
      _handleSimplMessage(simplMsgQueue->payload);

    nsHeadlightTask::fsm_mgr.runMachine();

    if (FEATURE_INACTIVITY_FLASH == 1)
      _checkForInactivity();
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

    bool moving = !vescData.moving && p_vescData.moving;
    bool stopped = vescData.moving && !p_vescData.moving;
    vescData = p_vescData;

    if (moving)
      fsm_mgr.trigger(Event::MOVING);
    else if (stopped)
      fsm_mgr.trigger(Event::STOPPED);
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
  void _checkForInactivity()
  {
    if (vescData.moving == false && sinceLastFlashed < inactivityLightsFlashInterval)
      return;

    sinceLastFlashed = 0;
    _simplMsg.message = SIMPL_HEADLIGHT_FLASH;
    simplMsgQueue->send(&_simplMsg);
  }
};
//------------------------------
//------------------------------------------------------------
HeadlightTask headlightTask;

namespace nsHeadlightTask
{
  void task1(void *parameters)
  {
    headlightTask.task(parameters);
  }

  elapsedMillis sinceActive = 0;

  void stateOff_OnEnter()
  {
    DEBUG("HeadlightTask: stateOff_OnEnter");
    headlightTask.turnLights(false);
  }

  void stateOn_OnEnter()
  {
    DEBUG("HeadlightTask: stateOn_OnEnter");
    headlightTask.turnLights(true);
  }

  void stateActive_OnEnter()
  {
    DEBUG("HeadlightTask: stateActive_OnEnter");
    sinceActive = 0;
  }

  void stateInactive_OnEnter()
  {
    DEBUG("HeadlightTask: stateInactive_OnEnter");
    // start flashing
  }

  void addTransitions()
  {
    fsm1->add_transition(&stateOff, &stateOn, Event::TOGGLE, NULL);
    fsm1->add_transition(&stateOn, &stateOff, Event::OFF, NULL);
    fsm1->add_transition(&stateOn, &stateOff, Event::TOGGLE, NULL);

    // fsm1->add_transition(&stateOff, &stateActive, Event::STOPPED, NULL);
    // fsm1->add_transition(&stateOff, &stateInactive, Event::STOPPED, NULL);
    // fsm1->add_transition(&stateInactive, &stateActive, Event::MOVING, NULL);

    // fsm1->add_timed_transition(&stateActive, &stateInactive, PERIOD_5s, NULL);
    // fsm1->add_transition(&stateActive, &stateActive, Event::MOVING, NULL);
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
