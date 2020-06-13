
#ifndef LedLightsLib_h
#include <LedLightsLib.h>
#endif

LedLightsLib footLight;

//------------------------------------------------------------------
enum FootLightEvent
{
  EV_MOVING,
  EV_STOPPED,
};

enum FootLightFsmEvent
{
  EV_FOOT_LIGHT_MOVING,
  EV_FOOT_LIGHT_STOPPED,
};

#define FOOT_LIGHT_PIXEL_PIN 5
#define NUM_PIXELS 8

xQueueHandle xFootLightEventQueue;

/* prototypes */
void footLightInit();
void addFootLightFsmTransitions();
void footLightMoving_OnEnter();
void footLightStopped_OnEnter();
void footLightFsmEvent(FootLightFsmEvent ev);
void PRINT_FOOT_LIGHT_STATE(const char *state_name);
void sendToFootLightEventQueue(FootLightEvent e);

State state_light_moving(footLightMoving_OnEnter, NULL, NULL);
State state_light_stopped(footLightStopped_OnEnter, NULL, NULL);

Fsm light_fsm(&state_light_stopped);

//--------------------------------------------------
void footLightTask_0(void *pvParameters)
{
  Serial.printf("footLightTask_0 running on core %d\n", xPortGetCoreID());

  footLightInit();

  addFootLightFsmTransitions();

  while (true)
  {
    FootLightEvent e;
    bool event_ready = xQueueReceive(xFootLightEventQueue, &e, pdMS_TO_TICKS(0)) == pdPASS;
    if (event_ready)
    {
      switch (e)
      {
      case FootLightEvent::EV_MOVING:
        footLightFsmEvent(FootLightFsmEvent::EV_FOOT_LIGHT_MOVING);
        break;
      case FootLightEvent::EV_STOPPED:
        footLightFsmEvent(FootLightFsmEvent::EV_FOOT_LIGHT_STOPPED);
        break;
      }
    }

    light_fsm.run_machine();

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
  light_fsm.add_transition(&state_light_moving, &state_light_stopped, EV_FOOT_LIGHT_STOPPED, NULL);
  light_fsm.add_transition(&state_light_stopped, &state_light_moving, EV_FOOT_LIGHT_MOVING, NULL);
}
//--------------------------------------------------

void PRINT_FOOT_LIGHT_STATE(const char *state_name)
{
#ifdef PRINT_FOOT_LIGHT_STATE_NAME
  Serial.printf("light-fsm: state ---> %s\n", state_name);
#endif
}
//--------------------------------------------------

void footLightFsmEvent(FootLightFsmEvent ev)
{
#ifdef PRINT_LIGHT_FSM_EVENT_TRIGGER
  switch (e)
  {
  case FootLightFsmEvent::EV_FOOT_LIGHT_MOVING:
    Serial.printf("footLightFsmEvent ---> EV_FOOT_LIGHT_MOVING\n");
    break;
  case FootLightFsmEvent::EV_FOOT_LIGHT_STOPPED:
    Serial.printf("footLightFsmEvent ---> EV_FOOT_LIGHT_STOPPED\n");
    break;
  default:
    Serial.printf("Unknown footLightFsmEvent ---> %d\n", ev);
    break;
  }
#endif
  light_fsm.trigger(ev);
}
//------------------------------------------------------------------

void footLightInit()
{
  footLight.initialise(FOOT_LIGHT_PIXEL_PIN, NUM_PIXELS, /*brightness*/ 100);
  footLight.setAll(footLight.COLOUR_DARK_RED);
}
//--------------------------------------------------

void footLightMoving_OnEnter()
{
  PRINT_FOOT_LIGHT_STATE("state_light_moving ---------------------- \n");
  footLight.setBrightness(FOOT_LIGHT_BRIGHTNESS);
  footLight.setAll(footLight.COLOUR_HEADLIGHT_WHITE);
}
//--------------------------------------------------

void footLightStopped_OnEnter()
{
  PRINT_FOOT_LIGHT_STATE("state_light_stopped ---------------------- \n");
  footLight.setBrightness(FOOT_LIGHT_BRIGHTNESS);
  footLight.setAll(footLight.COLOUR_BLUE);
}
//--------------------------------------------------