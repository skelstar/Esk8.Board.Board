

enum EventsEnum
{
  EV_POWERING_DOWN,
  EV_VESC_OFFLINE,
  EV_CONTROLLER_CONNECTED,
  EV_CONTROLLER_OFFLINE,
  EV_MOVING,
  EV_STOPPED,
  EV_MISSED_CONTROLLER_PACKET,
  EV_RECV_CONTROLLER_PACKET,
} event;

enum StateId
{
  STATE_POWERING_DOWN,
  STATE_VESC_OFFLINE,
  STATE_BOARD_MOVING,
  STATE_BOARD_STOPPED,
  STATE_CONTROLLER_OFFLINE,
};


elapsedMillis since_stopped;
bool showing_graph;

//------------------------------------------------------------------
State state_powering_down(
    STATE_POWERING_DOWN,
    []
    {
      DEBUG("state_powering_down ---------------------- \n");
      send_to_packet_controller(ReasonType::LAST_WILL);
    },
    NULL, NULL);
//------------------------------------------------------------------
State state_vesc_offline(
    STATE_VESC_OFFLINE,
    [] {
      DEBUG("state_vesc_offline ---------------------- \n");
      send_to_packet_controller(ReasonType::VESC_OFFLINE);
    },
    NULL, NULL);
//------------------------------------------------------------------
State state_board_moving(
    STATE_BOARD_MOVING,
    [] {
      DEBUG("state_board_moving ---------------------- \n");
      send_to_packet_controller(ReasonType::BOARD_MOVING);
      light.setAll(light.COLOUR_WHITE);
    },
    NULL, NULL);
//------------------------------------------------------------------
State state_board_stopped(
    STATE_BOARD_STOPPED,
    [] {
      DEBUG("state_board_stopped ---------------------- \n");
      send_to_packet_controller(ReasonType::BOARD_STOPPED);
      since_stopped = 0;
      showing_graph = false;
    },
    [] {
      if (since_stopped > 3000 && showing_graph == false)
      {
        showing_graph = true;
        DEBUG("state_board_stopped_show_graph ---------------------- \n");
        light.showBatteryGraph(getBatteryPercentage(vescdata.batteryVoltage) / 100.0);
      }
    },
    NULL);
//------------------------------------------------------------------
State state_controller_offline(
    STATE_CONTROLLER_OFFLINE,
    [] {
      DEBUG("state_controller_offline ---------------------- \n");
      vescdata.ampHours = vescdata.ampHours + 1; // number of times gone offline
    },
    NULL, NULL);
//------------------------------------------------------------------

Fsm fsm(&state_controller_offline);

void addFsmTransitions()
{
  // online/offline
  fsm.add_transition(&state_controller_offline, &state_board_stopped, EV_CONTROLLER_CONNECTED, NULL);
  fsm.add_transition(&state_board_stopped, &state_controller_offline, EV_CONTROLLER_OFFLINE, NULL);
  fsm.add_transition(&state_board_moving, &state_controller_offline, EV_CONTROLLER_OFFLINE, NULL);
  
  // stopped
  fsm.add_transition(&state_board_stopped, &state_powering_down, EV_POWERING_DOWN, NULL);
  fsm.add_transition(&state_board_stopped, &state_board_moving, EV_MOVING, NULL);
  fsm.add_transition(&state_board_stopped, &state_vesc_offline, EV_VESC_OFFLINE, NULL);

  // moving
  fsm.add_transition(&state_board_moving, &state_powering_down, EV_POWERING_DOWN, NULL);
  fsm.add_transition(&state_board_moving, &state_board_stopped, EV_STOPPED, NULL);
  fsm.add_transition(&state_board_moving, &state_vesc_offline, EV_VESC_OFFLINE, NULL);

  fsm.add_transition(&state_controller_offline, &state_vesc_offline, EV_RECV_CONTROLLER_PACKET, NULL);

  fsm.add_transition(&state_vesc_offline, &state_board_stopped, EV_STOPPED, NULL);
  fsm.add_transition(&state_vesc_offline, &state_board_moving, EV_MOVING, NULL);
}


void TRIGGER(uint8_t x, char *s)
{
  if (s != NULL)
  {
    Serial.printf("EVENT: %s\n", s);
  }
  fsm.trigger(x);
}

void TRIGGER(uint8_t x)
{
  switch (x)
  {
    case EV_POWERING_DOWN: Serial.printf("trigger: EV_POWERING_DOWN\n"); break;
    case EV_VESC_OFFLINE: Serial.printf("trigger: EV_VESC_OFFLINE\n"); break;
    case EV_CONTROLLER_CONNECTED: Serial.printf("trigger: EV_CONTROLLER_CONNECTED\n"); break;
    // case EV_CONTROLLER_OFFLINE: Serial.printf("trigger: EV_CONTROLLER_OFFLINE\n"); break;
    case EV_MOVING: Serial.printf("trigger: EV_MOVING\n"); break;
    case EV_STOPPED: Serial.printf("trigger: EV_STOPPED\n"); break;
    case EV_MISSED_CONTROLLER_PACKET: Serial.printf("trigger: EV_MISSED_CONTROLLER_PACKET\n"); break;
    case EV_RECV_CONTROLLER_PACKET: Serial.printf("trigger: EV_RECV_CONTROLLER_PACKET\n"); break;
    // default: Serial.printf("WARNING: unhandled trigger\n");
  }
  TRIGGER(x, NULL);
}
