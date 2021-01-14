
#ifndef LedLightsLib_h
#include <LedLightsLib.h>
#endif

LedLightsLib footLightPanel;

elapsedMillis sinceUpdatedBatteryGraph;

//------------------------------------------------------------------

namespace FootLight
{
  /* prototypes */

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
    return OUT_OF_RANGE;
  }

  bool taskReady = false;

  FsmManager<Event> footlightFsm;

  const char *name = "FOOTLIGHT";

  void printFsmState_cb(uint16_t id)
  {
    if (PRINT_FOOTLIGHT_STATE_NAME)
      Serial.printf(PRINT_STATE_FORMAT, name, getStateName(StateID(id)));
  }

  void printFsmTrigger_cb(uint16_t ev)
  {
    // if (PRINT_COMMS_FSM_TRIGGER)
    //   Serial.printf(PRINT_STATE_FORMAT, name, getEvent(Event(ev)));
  }

  //--------------------------------------------------
  State stateBoardBooted(
      [] {
        footlightFsm.printState(STATE_BOOTED);
        footLightPanel.setBrightness(FOOTLIGHT_BRIGHTNESS_STOPPED);
        footLightPanel.setAll(footLightPanel.COLOUR_BLUE);
      },
      NULL, NULL);
  //--------------------------------------------------
  State stateMoving(
      [] {
        footlightFsm.printState(STATE_MOVING);
        footLightPanel.setBrightness(FOOTLIGHT_BRIGHTNESS_MOVING);
        footLightPanel.setAll(footLightPanel.COLOUR_WHITE);
      },
      NULL, NULL);

  //--------------------------------------------------
  State stateStopped(
      [] {
        footlightFsm.printState(STATE_STOPPED);
        footLightPanel.setBrightness(FOOTLIGHT_BRIGHTNESS_STOPPED);
        float battPc = getBatteryPercentage(board_packet.batteryVoltage) / 100.0;
        footLightPanel.showBatteryGraph(battPc);
      },
      [] {
        if (sinceUpdatedBatteryGraph > 1000)
        {
          sinceUpdatedBatteryGraph = 0;
          footLightPanel.setBrightness(FOOTLIGHT_BRIGHTNESS_STOPPED);
          uint8_t battPc = getBatteryPercentage(board_packet.batteryVoltage);
          footLightPanel.showBatteryGraph(battPc);
        }
      },
      NULL);

  Fsm fsm(&stateBoardBooted);

  void addTransitions()
  {
    fsm.add_timed_transition(&stateBoardBooted, &stateStopped, 1000, NULL);
    fsm.add_transition(&stateBoardBooted, &stateStopped, STOPPED, NULL);
    fsm.add_transition(&stateMoving, &stateStopped, STOPPED, NULL);
    fsm.add_transition(&stateStopped, &stateMoving, MOVING, NULL);
  }

  //--------------------------------------------------

  void task(void *pvParameters)
  {
    Serial.printf("FootLight running on CORE_%d\n", xPortGetCoreID());

    footLightPanel.initialise(FOOTLIGHT_PIXEL_PIN, FOOTLIGHT_NUM_PIXELS, FOOTLIGHT_BRIGHTNESS_STOPPED);
    footLightPanel.setAll(footLightPanel.COLOUR_DARK_RED);

    footlightFsm.begin(&fsm);
    footlightFsm.setPrintStateCallback(printFsmState_cb);
    // footlightFsm.setPrintTriggerCallback(printFsmTrigger_cb);

    addTransitions();

    taskReady = true;

    while (true)
    {
      uint16_t ev = footlightQueue->read<FootLight::Event>();

      switch (ev)
      {
      case FootLight::MOVING:
      case FootLight::STOPPED:
        footlightFsm.trigger(FootLight::Event(ev));
        break;
      default:
        break;
      }

      footlightFsm.runMachine();

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }

  void createTask(uint8_t core, uint8_t priority)
  {
    xTaskCreatePinnedToCore(
        task,
        "FootLight",
        5000,
        NULL,
        priority,
        NULL,
        core);
  }
} // namespace FootLight
