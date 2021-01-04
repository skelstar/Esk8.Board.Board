
#ifndef LedLightsLib_h
#include <LedLightsLib.h>
#endif

LedLightsLib footLightPanel;

elapsedMillis sinceUpdatedBatteryGraph;

//------------------------------------------------------------------

#define PIN_05 5

#ifdef USING_M5STACK
#define FOOTLIGHT_PIXEL_PIN 1
#else
#define FOOTLIGHT_PIXEL_PIN PIN_05
#endif
#define NUM_PIXELS 8

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

  FsmManager<Event> footlightFsm;

  //--------------------------------------------------
  State stateBoardBooted(
      [] {
        footlightFsm.printState(STATE_BOOTED);
        footLightPanel.setBrightness(FOOTLIGHT_BRIGHTNESS_STOPPED);
        footLightPanel.setAll(footLightPanel.COLOUR_BLUE);
      },
      [] {
        if (sinceBoardBooted > 3000)
        {
          footlightFsm.trigger(Event::STOPPED);
        }
      },
      [] {

      });
  //--------------------------------------------------
  State stateMoving(
      [] {
        footlightFsm.printState(STATE_MOVING);
        footLightPanel.setBrightness(FOOTLIGHT_BRIGHTNESS_MOVING);
        footLightPanel.setAll(footLightPanel.COLOUR_HEADLIGHT_WHITE);
      });

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
    fsm.add_transition(&stateBoardBooted, &stateStopped, STOPPED, NULL);
    fsm.add_transition(&stateMoving, &stateStopped, STOPPED, NULL);
    fsm.add_transition(&stateStopped, &stateMoving, MOVING, NULL);
  }

  //--------------------------------------------------

  void task(void *pvParameters)
  {
    Serial.printf("FootLight running on CORE_%d\n", xPortGetCoreID());

    footLightPanel.initialise(FOOTLIGHT_PIXEL_PIN, NUM_PIXELS, FOOTLIGHT_BRIGHTNESS_STOPPED);
    // footLightPanel.setAll(footLightPanel.COLOUR_DARK_RED);

    // footlightFsm.begin(&fsm);

    // addTransitions();

    while (true)
    {
      // uint16_t ev = footlightQueue->read<FootLight::Event>();

      // switch (ev)
      // {
      // case FootLight::MOVING:
      // case FootLight::STOPPED:
      //   footlightFsm.trigger(FootLight::Event(ev));
      //   break;
      // default:
      //   break;
      // }

      // footlightFsm.runMachine();

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
//--------------------------------------------------

void PRINT_FOOTLIGHT_STATE(const char *state_name)
{
#ifdef PRINT_FOOTLIGHT_STATE_NAME
  Serial.printf("STATE: light-fsm ---> %s ---\n", state_name);
#endif
}
//--------------------------------------------------
