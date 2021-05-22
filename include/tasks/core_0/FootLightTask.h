#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <VescData.h>
#include <LedLightsLib.h>
#include <utils.h>
#include <constants.h>

#define FOOTLIGHT_TASK

#define FOOTLIGHT_NUM_PIXELS 8

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
  VescData vescData;

  bool printStateChange = false;

  LedLightsLib lightStrip;

private:
  Queue1::Manager<VescData> *vescDataQueue = nullptr;

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
    lightStrip.initialise(FOOTLIGHT_PIXEL_PIN, FOOTLIGHT_NUM_PIXELS, FOOTLIGHT_BRIGHTNESS_STOPPED);
    lightStrip.setAll(lightStrip.COLOUR_DARK_RED);
  }

  void doWork()
  {
    if (vescDataQueue->hasValue())
    {
      vescData = vescDataQueue->payload;
      nsFootlightTask::updateLights(vescData);
    }
  } // doWork

  void cleanup()
  {
    delete (vescDataQueue);
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
  const uint8_t UNKNOWN = 2;
  uint8_t moving = UNKNOWN;

  void updateLights(const VescData &vescData)
  {
    bool changed = moving != vescData.moving;
    if (changed && vescData.moving)
    {
      sinceUpdated = 0;
      footLightTask.lightStrip.setBrightness(FOOTLIGHT_BRIGHTNESS_MOVING);
      footLightTask.lightStrip.setAll(footLightTask.lightStrip.COLOUR_WHITE);
      if (footLightTask.printStateChange)
        Serial.printf("[TASK]:FootLight moving\n");
    }
    // stopped or need to update
    else if (changed || sinceUpdated > 1000)
    {
      sinceUpdated = 0;
      footLightTask.lightStrip.setBrightness(FOOTLIGHT_BRIGHTNESS_STOPPED);
      float battPc = getBatteryPercentage(vescData.batteryVoltage);
      footLightTask.lightStrip.showBatteryGraph(battPc);
      if (footLightTask.printStateChange)
        Serial.printf("[TASK]:FootLight stopped/show battery %.1fv pc=%.1f%%\n", vescData.batteryVoltage, battPc);
    }
    moving = vescData.moving;
  }
}
