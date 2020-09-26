
#ifndef LedLightsLib_h
#include <LedLightsLib.h>
#endif

LedLightsLib footLight;

elapsedMillis sinceUpdatedBatteryGraph;

//------------------------------------------------------------------
enum FootLightEvent
{
  QUEUE_EV_MOVING,
  QUEUE_EV_STOPPED,
  QUEUE_EV_OTA_MODE,
};

enum FootLightFsmEvent
{
  EV_FOOT_LIGHT_BOOTED,
  EV_FOOT_LIGHT_MOVING,
  EV_FOOT_LIGHT_STOPPED,
  EV_FOOT_LIGHT_ENTERED_OTA,
};

#define PIN_05 5

#ifdef USING_M5STACK
#define FOOT_LIGHT_PIXEL_PIN 1
#else
#define FOOT_LIGHT_PIXEL_PIN PIN_05
#endif
#define NUM_PIXELS 8

xQueueHandle xFootLightEventQueue;

Fsm *light_fsm;

/* prototypes */
void footLightInit();
void addFootLightFsmTransitions();

void footLightFsmEvent(FootLightFsmEvent ev);
void PRINT_FOOT_LIGHT_STATE(const char *state_name);
void sendToFootLightEventQueue(FootLightEvent e);

//--------------------------------------------------
State state_light_booted(
    [] {
      PRINT_FOOT_LIGHT_STATE("state_light_booted");
      footLight.setBrightness(FOOT_LIGHT_BRIGHTNESS_STOPPED);
      footLight.setAll(footLight.COLOUR_BLUE);
    },
    [] {
      if (sinceBoardBooted > 3000)
      {
        footLightFsmEvent(FootLightFsmEvent::EV_FOOT_LIGHT_STOPPED);
      }
    },
    [] {

    });
//--------------------------------------------------
State state_light_moving(
    [] {
      PRINT_FOOT_LIGHT_STATE("state_light_moving");
      footLight.setBrightness(FOOT_LIGHT_BRIGHTNESS_MOVING);
      footLight.setAll(footLight.COLOUR_HEADLIGHT_WHITE);
    });

//--------------------------------------------------
State state_light_stopped(
    [] {
      PRINT_FOOT_LIGHT_STATE("state_light_stopped onEnter");
      footLight.setBrightness(FOOT_LIGHT_BRIGHTNESS_STOPPED);
      float battPc = getBatteryPercentage(board_packet.batteryVoltage) / 100.0;
      footLight.showBatteryGraph(battPc); },
    [] {
      if (sinceUpdatedBatteryGraph > 1000)
      {
        sinceUpdatedBatteryGraph = 0;
        // if (!light_fsm->revisit())
        // {
        //   PRINT_FOOT_LIGHT_STATE("state_light_stopped onLoop");
        // }
        footLight.setBrightness(FOOT_LIGHT_BRIGHTNESS_STOPPED);
        uint8_t battPc = getBatteryPercentage(board_packet.batteryVoltage);
        footLight.showBatteryGraph(battPc);
      }
    },
    NULL);

//--------------------------------------------------
elapsedMillis sinceFlashedLightsForOta;
bool otaLightState = false;

State state_light_entered_ota(
    [] {
      PRINT_FOOT_LIGHT_STATE("state_light_entered_ota");
      footLight.setBrightness(FOOT_LIGHT_BRIGHTNESS_MOVING);
      footLight.setAll(footLight.COLOUR_HEADLIGHT_WHITE);
    },
    [] {
      // flash lights every second while in OTA mode
      if (sinceFlashedLightsForOta > 1000)
      {
        sinceFlashedLightsForOta = 0;
        PRINT_FOOT_LIGHT_STATE("state_light_entered_ota (loop)");
        footLight.setBrightness(30);
        footLight.setAll(otaLightState
                             ? footLight.COLOUR_OFF
                             : footLight.COLOUR_DARK_RED);
        otaLightState = !otaLightState;
      }
    },
    NULL);

//--------------------------------------------------
void footLightTask_0(void *pvParameters)
{
  Serial.printf("footLightTask_0 running on core %d\n", xPortGetCoreID());

  footLightInit();

  light_fsm = new Fsm(&state_light_booted);

  addFootLightFsmTransitions();

  while (true)
  {
    FootLightEvent e;
    bool event_ready = xQueueReceive(xFootLightEventQueue, &e, pdMS_TO_TICKS(0)) == pdPASS;
    if (event_ready)
    {
      switch (e)
      {
      case FootLightEvent::QUEUE_EV_MOVING:
        footLightFsmEvent(FootLightFsmEvent::EV_FOOT_LIGHT_MOVING);
        break;
      case FootLightEvent::QUEUE_EV_STOPPED:
        footLightFsmEvent(FootLightFsmEvent::EV_FOOT_LIGHT_STOPPED);
        break;
      case FootLightEvent::QUEUE_EV_OTA_MODE:
        footLightFsmEvent(FootLightFsmEvent::EV_FOOT_LIGHT_ENTERED_OTA);
        break;
      }
    }

    light_fsm->run_machine();

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//--------------------------------------------------

void sendToFootLightEventQueue(FootLightEvent e)
{
  xQueueSendToFront(xFootLightEventQueue, &e, pdMS_TO_TICKS(10));
}
//------------------------------------------------------------------

void addFootLightFsmTransitions()
{
  light_fsm->add_transition(&state_light_booted, &state_light_stopped, EV_FOOT_LIGHT_STOPPED, NULL);
  light_fsm->add_transition(&state_light_moving, &state_light_stopped, EV_FOOT_LIGHT_STOPPED, NULL);
  light_fsm->add_transition(&state_light_stopped, &state_light_moving, EV_FOOT_LIGHT_MOVING, NULL);
  // light_fsm->add_transition(&state_light_stopped, &state_light_entered_ota, EV_FOOT_LIGHT_ENTERED_OTA, NULL);
}
//--------------------------------------------------

void PRINT_FOOT_LIGHT_STATE(const char *state_name)
{
#ifdef PRINT_FOOT_LIGHT_STATE_NAME
  Serial.printf("STATE: light-fsm ---> %s ---\n", state_name);
#endif
}
//--------------------------------------------------

void footLightFsmEvent(FootLightFsmEvent ev)
{
#ifdef PRINT_LIGHT_FSM_EVENT_TRIGGER
  switch (e)
  {
  case FootLightFsmEvent::EV_FOOT_LIGHT_BOOTED:
    Serial.printf("footLightFsmEvent ---> EV_FOOT_LIGHT_MOVING\n");
    break;
  case FootLightFsmEvent::EV_FOOT_LIGHT_MOVING:
    Serial.printf("footLightFsmEvent ---> EV_FOOT_LIGHT_MOVING\n");
    break;
  case FootLightFsmEvent::EV_FOOT_LIGHT_STOPPED:
    Serial.printf("footLightFsmEvent ---> EV_FOOT_LIGHT_STOPPED\n");
    break;
  case FootLightFsmEvent::EV_FOOT_LIGHT_ENTERED_OTA:
    Serial.printf("footLightFsmEvent ---> EV_FOOT_LIGHT_ENTERED_OTA\n");
    break;
  default:
    Serial.printf("Unknown footLightFsmEvent ---> %d\n", ev);
    break;
  }
#endif
  light_fsm->trigger(ev);
}
//------------------------------------------------------------------

void footLightInit()
{
  footLight.initialise(FOOT_LIGHT_PIXEL_PIN, NUM_PIXELS, FOOT_LIGHT_BRIGHTNESS_STOPPED);
  footLight.setAll(footLight.COLOUR_DARK_RED);
}
//--------------------------------------------------