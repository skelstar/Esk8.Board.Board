

enum EventsEnum
{
  EV_WAITING_FOR_VESC,
  EV_POWERING_DOWN,
  EV_VESC_OFFLINE,
  EV_CONTROLLER_OFFLINE,
  EV_MOVING,
  EV_STOPPED,
  EV_MISSED_CONTROLLER_PACKET,
} event;

//------------------------------------------------------------------
State state_waiting_for_vesc([] {
  Serial.printf("state_waiting_for_vesc\n");
},
NULL, NULL);
//------------------------------------------------------------------
State state_powering_down([] {
  Serial.printf("state_powering_down\n");
},
NULL, NULL);
//------------------------------------------------------------------
State state_offline([] {
  Serial.printf("state_offline\n");
},
NULL, NULL);
//------------------------------------------------------------------
State state_board_moving([] {
  Serial.printf("state_board_moving\n");
},
NULL, NULL);
//------------------------------------------------------------------
State state_board_stopped([] {
  Serial.printf("state_board_stopped\n");
  if (vescOnline)
  {
    // lastStableVolts = vescdata.batteryVoltage;
  }
},
NULL, NULL);
//------------------------------------------------------------------
State state_controller_offline([] {
  Serial.printf("state_controller_offline\n");
},
NULL, NULL);
//------------------------------------------------------------------
Fsm fsm(&state_waiting_for_vesc);

void addFsmTransitions()
{
  event = EV_POWERING_DOWN;
  fsm.add_transition(&state_waiting_for_vesc, &state_powering_down, event, NULL);
  fsm.add_transition(&state_board_moving, &state_powering_down, event, NULL);
  fsm.add_transition(&state_board_stopped, &state_powering_down, event, NULL);

  event = EV_VESC_OFFLINE;
  fsm.add_transition(&state_board_moving, &state_offline, event, NULL);
  fsm.add_transition(&state_board_stopped, &state_offline, event, NULL);

  event = EV_MOVING;
  fsm.add_transition(&state_board_stopped, &state_board_moving, event, NULL);

  event = EV_STOPPED;
  fsm.add_transition(&state_waiting_for_vesc, &state_board_stopped, event, NULL);
  fsm.add_transition(&state_board_moving, &state_board_stopped, event, NULL);

  event = EV_CONTROLLER_OFFLINE;
  fsm.add_transition(&state_waiting_for_vesc, &state_controller_offline, event, NULL);
  fsm.add_transition(&state_board_moving, &state_controller_offline, event, NULL);
  fsm.add_transition(&state_board_stopped, &state_controller_offline, event, NULL);

  event = EV_MISSED_CONTROLLER_PACKET;
  fsm.add_transition(&state_waiting_for_vesc, &state_controller_offline, event, [] {
    DEBUG("Zero throttle!");
    vesc.setNunchuckValues(127, 127, 0, 0);
  });
  fsm.add_transition(&state_board_moving, &state_controller_offline, event, [] {
    DEBUG("Zero throttle!");
    vesc.setNunchuckValues(127, 127, 0, 0);
  });
  fsm.add_transition(&state_board_stopped, &state_controller_offline, event, [] {
    DEBUG("Zero throttle!");
    vesc.setNunchuckValues(127, 127, 0, 0);
  });
}

