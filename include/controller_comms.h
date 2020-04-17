
elapsedMillis since_requested;

void handle_first_packet();
void handle_config_packet();
uint8_t manage_throttle(uint8_t controller_throttle);

enum CommsStateEvent
{
  EV_VESC_SUCCESS,
  EV_VESC_FAILED,
  EV_CTRLR_PKT,
  EV_CTRLR_TIMEOUT,
};

/* prototypes */
bool sendPacketToController();
void sendCommsStateEvent(CommsStateEvent ev);
void printState(char *stateName);
void processControlPacket();
void processConfigPacket();

//------------------------------------------------------
void packet_available_cb(uint16_t from_id, uint8_t type)
{
  since_last_controller_packet = 0;

  if (type == PacketType::CONTROL)
  {
    processControlPacket();
  }
  else if (type == PacketType::CONFIG)
  {
    processConfigPacket();
  }

  sendCommsStateEvent(EV_CTRLR_PKT);
}

//------------------------------------------------------
void processControlPacket()
{
  prevControllerPacket = controller_packet;

  uint8_t buff[sizeof(ControllerData)];
  nrf24.read_into(buff, sizeof(ControllerData));
  memcpy(&controller_packet, &buff, sizeof(ControllerData));

#ifdef SEND_TO_VESC
  send_to_vesc(controller_packet.throttle, /*cruise*/ false);
#endif

  board_packet.id = controller_packet.id;
  board_packet.reason = ReasonType::RESPONSE;

  if (false == sendPacketToController())
  {
  }

#ifdef PRINT_THROTTLE
  if (prevControllerPacket.throttle != controller_packet.throttle)
  {
    DEBUGVAL(controller_packet.id, controller_packet.throttle);
  }
#endif
}

//------------------------------------------------------
void processConfigPacket()
{
  board_packet.id = controller_config.id;
  board_packet.reason = ReasonType::RESPONSE;
  bool success = sendPacketToController();

  handle_config_packet();
}

//------------------------------------------------------
bool sendPacketToController()
{
  uint8_t buff[sizeof(VescData)];
  memcpy(&buff, &board_packet, sizeof(VescData));

  bool success = nrf24.send_packet(/*to*/ COMMS_CONTROLLER, 0, buff, sizeof(VescData));
  if (false == success)
  {
    DEBUGVAL(success);
  }
#ifdef PRINT_SEND_TO_CONTROLLER
  DEBUGVAL("sending", board_packet.id);
#endif

  return success;
}
//------------------------------------------------------
void handle_first_packet()
{
}
//------------------------------------------------------
void handle_config_packet()
{
  uint8_t buff[sizeof(ControllerConfig)];
  nrf24.read_into(buff, sizeof(ControllerConfig));
  memcpy(&controller_config, &buff, sizeof(ControllerConfig));
  DEBUGVAL("***config***", controller_config.send_interval);
}
//------------------------------------------------------
bool controller_timed_out()
{
  if (controller_config.send_interval == 0)
  {
    return false;
  }
  return since_last_controller_packet > controller_config.send_interval + 100;
}
//------------------------------------------------------

#ifndef FSM_H
#include <Fsm.h>
#endif

State state_comms_offline([] {
  printState("state_comms_offline");
  controller_connected = false;
},
                          NULL, NULL);

State state_ctrlr_online([] {
  printState("state_ctrlr_online");
  controller_connected = true;
},
                         NULL, NULL);

State state_vesc_online([] {
  printState("state_vesc_online");
},
                        NULL, NULL);

State state_ctrlr_vesc_online([] {
  printState("state_ctrlr_vesc_online");
},
                              NULL, NULL);

void reportOffline()
{
  printState("timed transition going offline");
}

Fsm commsFsm(&state_comms_offline);

void addCommsFsmTransitions()
{
  // EV_CTRLR_PKT
  commsFsm.add_transition(&state_comms_offline, &state_ctrlr_online, 2, NULL);
  commsFsm.add_transition(&state_vesc_online, &state_ctrlr_vesc_online, 2, NULL);
  // EV_VESC_SUCCESS
  commsFsm.add_transition(&state_comms_offline, &state_vesc_online, EV_VESC_SUCCESS, NULL);
  commsFsm.add_transition(&state_ctrlr_online, &state_ctrlr_vesc_online, EV_VESC_SUCCESS, NULL);
  // EV_CTRLR_TIMEOUT
  commsFsm.add_transition(&state_ctrlr_vesc_online, &state_vesc_online, EV_CTRLR_TIMEOUT, NULL);
  commsFsm.add_transition(&state_ctrlr_online, &state_comms_offline, EV_CTRLR_TIMEOUT, NULL);
  // EV_VESC_FAILED
  commsFsm.add_transition(&state_ctrlr_vesc_online, &state_ctrlr_online, EV_VESC_FAILED, NULL);
  commsFsm.add_transition(&state_vesc_online, &state_comms_offline, EV_VESC_FAILED, NULL);
}

char *commsEventToString(CommsStateEvent ev)
{
  switch (ev)
  {
  case EV_VESC_SUCCESS:
    return "EV_VESC_SUCCESS";
  case EV_VESC_FAILED:
    return "EV_VESC_TIMEOUT";
  case EV_CTRLR_PKT:
    return "EV_CTRLR_PKT";
  case EV_CTRLR_TIMEOUT:
    return "EV_CTRLR_TIMEOUT";
  default:
    return "Unhandled ev!";
  }
}

void printState(char *stateName)
{
#ifdef PRINT_COMMS_STATE
  Serial.printf("COMMS_STATE: %s\n", stateName);
#endif
}

void sendCommsStateEvent(CommsStateEvent ev)
{
  commsFsm.trigger(ev);
#ifdef PRINT_COMMS_STATE_EVENT
  switch (ev)
  {
  case EV_CTRLR_PKT:
#ifdef SUPPRESS_EV_CTRLR_PKT
    return;
#endif
  case EV_VESC_SUCCESS:
#ifdef SUPPRESS_EV_VESC_SUCCESS
    return;
#endif
  case EV_VESC_FAILED:
  case EV_CTRLR_TIMEOUT:
#ifdef SUPPRESS_EV_CTRLR_TIMEOUT
    return;
#endif
    break;
  default:
    DEBUG("Unhandled ev");
    break;
  }

  Serial.printf("-> COMMS_STATE EVENT -> %s\n", commsEventToString(ev));
#endif
}