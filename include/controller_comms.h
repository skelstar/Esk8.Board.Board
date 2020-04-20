
elapsedMillis since_requested;

void handle_first_packet();
void handle_config_packet();
uint8_t manage_throttle(uint8_t controller_throttle);

enum CommsStateEvent
{
  EV_NONE,
  EV_VESC_SUCCESS,
  EV_VESC_FAILED,
  EV_CTRLR_PKT,
  EV_CTRLR_TIMEOUT,
};

/* prototypes */
bool sendPacketToController();
void sendCommsStateEvent(CommsStateEvent ev);
char *commsEventToString(CommsStateEvent ev);
void printCommsState(const char *stateName, const char *event);
void printCommsState(char *stateName);
void processControlPacket();
void processConfigPacket();
uint16_t getMissedPacketCount();

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
  // backup old controller_packet
  prevControllerPacket = controller_packet;

  uint8_t buff[sizeof(ControllerData)];
  nrf24.read_into(buff, sizeof(ControllerData));
  memcpy(&controller_packet, &buff, sizeof(ControllerData));

#ifdef SEND_TO_VESC
  send_to_vesc(controller_packet.throttle, /*cruise*/ false);
#endif

  board_packet.id = controller_packet.id;
  board_packet.reason = ReasonType::RESPONSE;

  uint16_t missedPackets = getMissedPacketCount();
  if (missedPackets > 0)
  {
    DEBUGVAL(missedPackets);
  }
  board_packet.missedPackets += missedPackets;

#ifdef BUTTON_MISS_PACKETS
  if (false == sendPacketToController() || button0.isPressed())
#else
  if (false == sendPacketToController())
#endif
  {
    board_packet.unsuccessfulSends++;
    DEBUGVAL(board_packet.unsuccessfulSends);
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

  board_packet.missedPackets = 0;
  board_packet.unsuccessfulSends = 0;

  handle_config_packet();
}

//------------------------------------------------------
bool sendPacketToController()
{
  uint8_t buff[sizeof(VescData)];
  memcpy(&buff, &board_packet, sizeof(VescData));

#ifdef PRINT_SEND_TO_CONTROLLER
  DEBUGVAL("sending", board_packet.id);
#endif
  return nrf24.send_packet(/*to*/ COMMS_CONTROLLER, 0, buff, sizeof(VescData));
}

//------------------------------------------------------
uint16_t getMissedPacketCount()
{
  return controller_packet.id > 0
             ? (controller_packet.id - prevControllerPacket.id) - 1
             : 0;
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

CommsStateEvent lastCommsEvent = EV_NONE;

State state_comms_offline([] {
  printCommsState("state_comms_offline", commsEventToString(lastCommsEvent));
  controller_connected = false;
});

State state_ctrlr_online([] {
  printCommsState("state_ctrlr_online", commsEventToString(lastCommsEvent));
  controller_connected = true;
});

State state_vesc_online([] {
  printCommsState("state_vesc_online", commsEventToString(lastCommsEvent));
});

State state_ctrlr_vesc_online([] {
  printCommsState("state_ctrlr_vesc_online", commsEventToString(lastCommsEvent));
});

//------------------------------------------------------
Fsm commsFsm(&state_comms_offline);

//------------------------------------------------------
void addCommsFsmTransitions()
{
  // state_offline
  commsFsm.add_transition(&state_comms_offline, &state_ctrlr_online, EV_CTRLR_PKT, NULL);
  commsFsm.add_transition(&state_comms_offline, &state_vesc_online, EV_VESC_SUCCESS, NULL);

  // state_ctrlr_online
  commsFsm.add_transition(&state_ctrlr_online, &state_comms_offline, EV_CTRLR_TIMEOUT, NULL);
  commsFsm.add_transition(&state_ctrlr_online, &state_ctrlr_vesc_online, EV_VESC_SUCCESS, NULL);
  // state_vesc_online
  commsFsm.add_transition(&state_vesc_online, &state_ctrlr_vesc_online, EV_CTRLR_PKT, NULL);
  commsFsm.add_transition(&state_vesc_online, &state_comms_offline, EV_VESC_FAILED, NULL);
  // state_ctrlr_vesc_online
  commsFsm.add_transition(&state_ctrlr_vesc_online, &state_vesc_online, EV_CTRLR_TIMEOUT, NULL);
  commsFsm.add_transition(&state_ctrlr_vesc_online, &state_ctrlr_online, EV_VESC_FAILED, NULL);
}

//------------------------------------------------------
char *commsEventToString(CommsStateEvent ev)
{
  switch (ev)
  {
  case EV_VESC_SUCCESS:
    return "EV_VESC_SUCCESS";
  case EV_VESC_FAILED:
    return "EV_VESC_FAILED";
  case EV_CTRLR_PKT:
    return "EV_CTRLR_PKT";
  case EV_CTRLR_TIMEOUT:
    return "EV_CTRLR_TIMEOUT";
  case EV_NONE:
    return "EV_NONE";
  default:
    return "Unhandled ev!";
  }
}
//------------------------------------------------------

void printCommsState(const char *stateName, const char *event)
{
#ifdef PRINT_COMMS_STATE
  Serial.printf("COMMS_STATE: %s --> %s\n", stateName, event);
#endif
}
//------------------------------------------------------

void printCommsState(const char *stateName)
{
#ifdef PRINT_COMMS_STATE
  Serial.printf("COMMS_STATE: %s\n", stateName);
#endif
}
//------------------------------------------------------

void sendCommsStateEvent(CommsStateEvent ev)
{
  lastCommsEvent = ev;
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
//------------------------------------------------------