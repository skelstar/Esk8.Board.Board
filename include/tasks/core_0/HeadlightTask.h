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

  enum OutputPortFunctionBits
  {
    LIGHT_FRONT = 7,
    LIGHT_REAR = 15,
  };

private:
  Queue1::Manager<VescData> *vescDataQueue = nullptr;
  Queue1::Manager<I2CPinsType> *i2cPinsQueue = nullptr;

  uint8_t lightState = LIGHTS_OFF;

  VescData vescData;
  I2CPinsType m_i2cPins;

public:
  HeadlightTask() : TaskBase("HeadlightTask", /*stacksize*/ 10000)
  {
    _core = CORE_0;
  }
  //------------------------------
  void turnLights(bool on)
  {
    if (on)
    {
      BIT_SET(m_i2cPins.outputs, OutputPortFunctionBits::LIGHT_FRONT);
      BIT_SET(m_i2cPins.outputs, OutputPortFunctionBits::LIGHT_REAR);
    }
    else
    {
      BIT_CLEAR(m_i2cPins.outputs, OutputPortFunctionBits::LIGHT_FRONT);
      BIT_CLEAR(m_i2cPins.outputs, OutputPortFunctionBits::LIGHT_REAR);
    }
    i2cPinsQueue->send(&m_i2cPins);
  }
  //------------------------------
  void flashLights()
  {
  }

private:
  //------------------------------
  void _initialise()
  {
    vescDataQueue = createQueue<VescData>("(HeadlightTask) vescDataQueue");
    vescDataQueue->printMissedPacket = false; // happens a lot since slow doWorkInterval
    i2cPinsQueue = createQueue<I2CPinsType>("(HeadlightTask) i2cPinsQueue");

    using namespace nsHeadlightTask;

    fsm1 = new Fsm(&stateOff);
    fsm_mgr.begin(fsm1);
    fsm_mgr.setPrintStateCallback(_printState);
    fsm_mgr.setPrintTriggerCallback(_printTrigger);

    m_i2cPins.inputs = 0x00;

    addTransitions();
  }
  //------------------------------
  void doWork()
  {
    if (vescDataQueue->hasValue())
      _handleVescData(vescDataQueue->payload);

    if (i2cPinsQueue->hasValue())
      _handleI2CPins(i2cPinsQueue->payload);

    nsHeadlightTask::fsm_mgr.runMachine();
  }
  //------------------------------
  void cleanup()
  {
    delete (vescDataQueue);
    delete (i2cPinsQueue);
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
  void _handleI2CPins(I2CPinsType &payload)
  {
    using namespace nsHeadlightTask;

    uint16_t og_i2cInputs = m_i2cPins.inputs;
    m_i2cPins = payload;

    bool changed = og_i2cInputs != payload.inputs;
    if (changed)
    {
      if (BIT_CHANGED(payload.inputs, og_i2cInputs, 7) && BIT_HIGH(payload.inputs, 7))
      {
        fsm_mgr.trigger(Event::TOGGLE);
      }
    }
  }

  //------------------------------
  static void _printState(uint16_t st)
  {
    Serial.printf("HeadlightTask state: %s\n", nsHeadlightTask::getState(st));
  }

  static void _printTrigger(uint16_t ev)
  {
    Serial.printf("HeadlightTask trigger: %s\n", nsHeadlightTask::getEvent(ev));
  }
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
    bool isInactive = sinceStopped > FEATURE_INACTIVITY_TIMEOUT_IN_SECONDS * SECONDS;

    if (FEATURE_INACTIVITY_FLASH && isInactive)
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
