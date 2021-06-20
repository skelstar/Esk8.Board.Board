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

elapsedMillis sinceUpdatedBatteryGraph;

//------------------------------------------------------------------
namespace nsFootlightTask
{
  // prototypes
  void updateLights(const VescData &vescData);
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

  VescData m_vescData;
  ControllerData m_controllerData;
  elapsedMillis sinceLastControllerData = 1000;

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
  }

  void doWork()
  {
    using namespace nsFootlightTask;

    if (vescDataQueue->hasValue())
      _handleVescData(vescDataQueue->payload);

    if (controllerQueue->hasValue())
      _handleControllerData(controllerQueue->payload);

    // bool connected = sinceLastControllerData > 400;

    // if (connected)
    //   nsFootlightTask::state = nsFootlightTask::DISCONNECTED;

    updateLights(m_vescData);
  }

  void cleanup()
  {
    delete (vescDataQueue);
    delete (controllerQueue);
  }

private:
  void _handleVescData(const VescData &payload)
  {
    m_vescData = vescDataQueue->payload;
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

  void updateLights(const VescData &vescData)
  {
    if (!vescData.moving || sinceUpdated > 1000)
    {
      sinceUpdated = 0;
      footLightTask.lightStrip.setBrightness(FOOTLIGHT_BRIGHTNESS_STOPPED);
      float percent = getBatteryPercentage(vescData.batteryVoltage);
      footLightTask.lightStrip.showBatteryGraph(percent);
      if (footLightTask.printStateChange)
        Serial.printf("[TASK]:FootLight stopped/show battery %.1fv pc=%.1f%%\n", vescData.batteryVoltage, percent);
    }
    moving = vescData.moving;
  }
}
