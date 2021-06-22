#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <VescData.h>
#include <LedLightsLib.h>
#include <utils.h>
#include <constants.h>

#define FOOTLIGHT_TASK

#define FOOTLIGHT_NUM_PIXELS 10

//------------------------------------------------------------------
namespace nsFootlightTask
{
  // prototypes
  void updateLights(bool moving, bool connected, float batteryVolts);
  void addTransitions();

  float batteryVolts;
}
//------------------------------------------------------------------

class FootLightTask : public TaskBase
{
  enum StateID
  {
    STATE_BOOTED = 0,
    STATE_STOPPED,
    STATE_MOVING,
  };

  const char *getStateName(uint16_t id)
  {
    switch (id)
    {
    case STATE_BOOTED:
      return "BOOTED";
    case STATE_STOPPED:
      return "STOPPED";
    case STATE_MOVING:
      return "MOVING";
    }
    return "OUT OF RANGE: FootlightTask getStateName";
  }

public:
  bool printStateChange = false;

  LedLightsLib lightStrip;

private:
  Queue1::Manager<VescData> *vescDataQueue = nullptr;
  Queue1::Manager<ControllerData> *controllerQueue = nullptr;

  ControllerData m_controllerData;
  elapsedMillis sinceLastControllerData = 1000;
  elapsedMillis sinceUpdatedBatteryGraph;
  VescData m_vescData;
  bool m_controllerConnected = sinceLastControllerData > 500;

public:
  FootLightTask() : TaskBase("FootLightTask", 3000)
  {
    _core = CORE_0;
  }

private:
  void _initialise()
  {
    vescDataQueue = createQueue<VescData>("vescDataQueue");
    vescDataQueue->printMissedPacket = false;

    controllerQueue = createQueue<ControllerData>("controllerQueue");
    controllerQueue->printMissedPacket = true;

    lightStrip.initialise(FOOTLIGHT_PIXEL_PIN, FOOTLIGHT_NUM_PIXELS, FOOTLIGHT_BRIGHTNESS_STOPPED);
    lightStrip.setAll(lightStrip.COLOUR_DARK_RED);

    nsFootlightTask::addTransitions();
  }

  void doWork()
  {
    using namespace nsFootlightTask;

    if (vescDataQueue->hasValue())
      _handleVescData(vescDataQueue->payload);

    if (controllerQueue->hasValue())
      _handleControllerData(controllerQueue->payload);

    if (sinceUpdatedBatteryGraph > 500)
    {
      sinceUpdatedBatteryGraph = 0;
      bool connected = sinceLastControllerData < 500;
      updateLights(m_vescData.moving, connected, m_vescData.batteryVoltage);
    }
  }

  void cleanup()
  {
    delete (vescDataQueue);
    delete (controllerQueue);
  }

private:
  void _handleVescData(VescData &payload)
  {
    m_vescData = payload;
  }

  void _handleControllerData(const ControllerData &payload)
  {
    sinceLastControllerData = 0;
  }
};
//-------------------------------------------------------
static FootLightTask footLightTask;

namespace nsFootlightTask
{
  elapsedMillis sinceUpdated = 0;

  namespace
  {
    float m_batteryVoltage = 0.0;
  }

  // prototypes
  void showBatteryLights(float batteryVoltage);
  void blankBatteryLights();

  void task1(void *parameters)
  {
    footLightTask.task(parameters);
  }

  const uint8_t STOPPED = 0;
  const uint8_t MOVING = 1;
  const uint8_t DISCONNECTED = 2;
  const uint8_t UNKNOWN = 3;
  uint8_t moving = UNKNOWN;
  uint8_t state = DISCONNECTED;

  enum Trigger
  {
    TR_STOPPED,
    TR_MOVING,
    TR_DISCONNECTED,
  };

  State stateStopped(
      []
      {
        showBatteryLights(m_batteryVoltage);
      },
      NULL, NULL);
  State stateMoving(
      []
      {
        Serial.printf("[State] Moving\n");
      },
      NULL, NULL);

  bool showingBattVolts = true;

  void stateDisconnected_onEnter()
  {
    if (showingBattVolts)
      showBatteryLights(m_batteryVoltage);
    else
      blankBatteryLights();
  }
  void stateDisconnected_onExit()
  {
    showingBattVolts = !showingBattVolts; // toggle for next loop
  }
  State stateDisconnected(stateDisconnected_onEnter, NULL, stateDisconnected_onExit);

  Fsm fsm1(&stateDisconnected);

  void addTransitions()
  {
    fsm1.add_transition(&stateStopped, &stateMoving, TR_MOVING, NULL);
    fsm1.add_transition(&stateMoving, &stateStopped, TR_STOPPED, NULL);

    fsm1.add_transition(&stateStopped, &stateDisconnected, TR_DISCONNECTED, NULL);
    fsm1.add_transition(&stateMoving, &stateDisconnected, TR_DISCONNECTED, NULL);

    // toggle when in Disconnected state
    fsm1.add_timed_transition(&stateDisconnected, &stateDisconnected, 1000, NULL);
    fsm1.add_transition(&stateDisconnected, &stateStopped, TR_STOPPED, NULL);
    fsm1.add_transition(&stateDisconnected, &stateMoving, TR_MOVING, NULL);
  }

  void showBatteryLights(float batteryVoltage)
  {
    footLightTask.lightStrip.setBrightness(FOOTLIGHT_BRIGHTNESS_STOPPED);
    float percent = getBatteryPercentage(batteryVoltage);
    footLightTask.lightStrip.showBatteryGraph(percent);
  }
  void blankBatteryLights()
  {
    const uint32_t BLACK = Adafruit_NeoPixel::Color(0, 0, 0, 0);
    footLightTask.lightStrip.setAll(BLACK);
  }

  void updateLights(bool moving, bool connected, float batteryVolts)
  {
    m_batteryVoltage = batteryVolts;

    if (connected == false)
      fsm1.trigger(Trigger::TR_DISCONNECTED);
    else if (moving)
      fsm1.trigger(Trigger::TR_MOVING);
    else
      fsm1.trigger(Trigger::TR_STOPPED);

    fsm1.run_machine();
  }
}
