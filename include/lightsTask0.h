
enum LightFsmEvent
{
  EV_LIGHT_MOVING,
  EV_LIGHT_STOPPED,
};

void PRINT_FOOT_LIGHT_STATE(const char *state_name);

//------------------------------------------------------------------
State state_light_moving(
    [] {
      PRINT_FOOT_LIGHT_STATE("state_light_moving ---------------------- \n");
      // light.setBrightness(HEADLIGHT_BRIGHTNESS);

      // light.setAll(light.COLOUR_HEADLIGHT_WHITE, 0, 12 - 1);
      // light.setAll(light.COLOUR_OFF, 12, 12 + 10 - 1);
      // light.setAll(light.COLOUR_HEADLIGHT_WHITE, 12 + 10, 12 + 10 + 12);
    },
    NULL, NULL);
//------------------------------------------------------------------
State state_light_wait_before_bargraph(
    [] {
      PRINT_FOOT_LIGHT_STATE("state_light_wait_before_bargraph ---------------------- \n");
    },
    NULL, NULL);
//------------------------------------------------------------------
State state_light_stopped(
    [] {
      PRINT_FOOT_LIGHT_STATE("state_light_stopped ---------------------- \n");
#ifdef LIGHTS_BAR_GRAPH_MODE
  // light.setBrightness(10);
  // light.setAll(light.COLOUR_OFF);
  // light.showBatteryGraph(getBatteryPercentage(board_packet.batteryVoltage) / 100.0, 12, 12 + 10);
#else
  // light.setBrightness(HEADLIGHT_BRIGHTNESS);
  // light.setAll(light.COLOUR_WHITE);
#endif
    },
    NULL, NULL);
//------------------------------------------------------------------

Fsm light_fsm(&state_light_wait_before_bargraph);

void add_light_fsm_transistions()
{
  // moving -> wait
  light_fsm.add_transition(
      &state_light_moving,
      &state_light_wait_before_bargraph,
      EV_LIGHT_STOPPED,
      NULL);
  // waiting -> stopped (bargraph)
  light_fsm.add_timed_transition(
      &state_light_wait_before_bargraph,
      &state_light_stopped,
      3000,
      NULL);
  // ... -> moving
  light_fsm.add_transition(
      &state_light_wait_before_bargraph,
      &state_light_moving,
      EV_LIGHT_MOVING,
      NULL);
  light_fsm.add_transition(
      &state_light_stopped,
      &state_light_moving,
      EV_LIGHT_MOVING,
      NULL);
}
//--------------------------------------------------
void PRINT_FOOT_LIGHT_STATE(const char *state_name)
{
#ifdef PRINT_LIGHT_FSM_STATE_NAME
  Serial.printf("light-fsm: state ---> %s\n", state_name);
#endif
}

void light_fsm_event(LightFsmEvent ev)
{
#ifdef PRINT_LIGHT_FSM_EVENT_TRIGGER
  Serial.printf("light-fsm: event ---> %d\n", ev);
#endif
  light_fsm.trigger(ev);
}
//--------------------------------------------------
void lightTask_0(void *pvParameters)
{
  Serial.printf("lightTask_0 running on core %d\n", xPortGetCoreID());

  // light_init();

  add_light_fsm_transistions();

  while (true)
  {
    // LightsEvent e;
    // bool event_ready = xQueueReceive(xLightsEventQueue, &e, pdMS_TO_TICKS(0)) == pdPASS;
    // if (event_ready)
    // {
    //   switch (e)
    //   {
    //   case LightsEvent::EV_MOVING:
    //     light_fsm_event(EV_LIGHT_MOVING);
    //     break;
    //   case LightsEvent::QUEUE_EV_STOPPED:
    //     light_fsm_event(EV_LIGHT_STOPPED);
    //     break;
    //   }
    // }

    light_fsm.run_machine();

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//--------------------------------------------------
