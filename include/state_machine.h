

enum EventsEnum
{
  EV_POWERING_DOWN,
  EV_VESC_OFFLINE,
  EV_CONTROLLER_OFFLINE,
  EV_MOVING,
  EV_STOPPED,
  EV_MISSED_CONTROLLER_PACKET,
  EV_RECV_CONTROLLER_PACKET,
} event;

elapsedMillis since_stopped;
bool showing_graph;

//------------------------------------------------------------------
State state_powering_down([] {
  Serial.printf("state_powering_down ---------------------- \n");
  send_to_packet_controller_1(ReasonType::LAST_WILL);
},
NULL, NULL);
//------------------------------------------------------------------
State state_vesc_offline([] {
  Serial.printf("state_vesc_offline ---------------------- \n");
},
NULL, NULL);
//------------------------------------------------------------------
State state_board_moving([] {
  Serial.printf("state_board_moving ---------------------- \n");
  send_to_packet_controller_1(ReasonType::BOARD_MOVING);
  light.setAll(light.COLOUR_WHITE);
},
NULL, NULL);
//------------------------------------------------------------------
State state_board_stopped(
  [] {
    Serial.printf("state_board_stopped ---------------------- \n");
    send_to_packet_controller_1(ReasonType::BOARD_STOPPED);
    since_stopped = 0;
    showing_graph = false;
  },
  [] {
    if (since_stopped > 3000 && showing_graph == false)
    {
      showing_graph = true;
      Serial.printf("state_board_stopped_show_graph ---------------------- \n");
      light.showBatteryGraph(getBatteryPercentage(vescdata.batteryVoltage)/100.0);
    }
  },
  NULL);
//------------------------------------------------------------------
State state_controller_offline([] {
  Serial.printf("state_controller_offline ---------------------- \n");
  vescdata.ampHours = vescdata.ampHours + 1;  // number of times gone offline
},
NULL, NULL);
//------------------------------------------------------------------
void handle_missing_packets()
{
  vescdata.missing_packets = vescdata.missing_packets + controller_packet.id - (old_packet.id + 1);

  uint8_t number_packets_missed = controller_packet.id - (old_packet.id + 1);
  if (number_packets_missed >= MISSED_PACKET_COUNT_THAT_ZEROS_THROTTLE)
  {
    DEBUG("Zero throttle!");
    vesc.setNunchuckValues(127, 127, 0, 0);

    if (!vescdata.moving)
    {
      send_to_packet_controller_1(ReasonType::BOARD_STOPPED);
    }
  }
}
//------------------------------------------------------------------

Fsm fsm(&state_controller_offline);

void addFsmTransitions()
{

  // stopped
  fsm.add_transition(&state_board_stopped, &state_powering_down, EV_POWERING_DOWN, NULL);
  fsm.add_transition(&state_board_stopped, &state_board_moving, EV_MOVING, NULL);
  fsm.add_transition(&state_board_stopped, &state_vesc_offline, EV_VESC_OFFLINE, NULL);
  fsm.add_transition(&state_board_stopped, &state_controller_offline, EV_CONTROLLER_OFFLINE, NULL);

  // moving
  fsm.add_transition(&state_board_moving, &state_powering_down, EV_POWERING_DOWN, NULL);
  fsm.add_transition(&state_board_moving, &state_board_stopped, EV_STOPPED, NULL);
  fsm.add_transition(&state_board_moving, &state_vesc_offline, EV_VESC_OFFLINE, NULL);
  fsm.add_transition(&state_board_moving, &state_controller_offline, EV_CONTROLLER_OFFLINE, NULL);

  fsm.add_transition(&state_controller_offline, &state_vesc_offline, EV_RECV_CONTROLLER_PACKET, NULL);

  fsm.add_transition(&state_vesc_offline, &state_board_stopped, EV_STOPPED, NULL);
  fsm.add_transition(&state_vesc_offline, &state_board_moving, EV_MOVING, NULL);
}

