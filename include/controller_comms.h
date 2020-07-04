

uint8_t manage_throttle(uint8_t controller_throttle);

enum CommsStateEvent
{
  EV_NONE,
  EV_VESC_SUCCESS,
  EV_VESC_FAILED,
  EV_CTRLR_PKT,
  EV_CTRLR_TIMEOUT,
};

elapsedMillis since_requested;

/* prototypes */
bool sendPacketToController(ReasonType reason = RESPONSE);
void sendCommsStateEvent(CommsStateEvent ev);
const char *commsEventToString(int ev);
void processControlPacket();
void processConfigPacket();

//------------------------------------------------------
void packet_available_cb(uint16_t from_id, uint8_t type)
{
  sinceLastControllerPacket = 0;

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
  ControllerData controller_packet;

  uint8_t buff[sizeof(ControllerData)];
  nrf24.read_into(buff, sizeof(ControllerData));
  memcpy(&controller_packet, &buff, sizeof(ControllerData));
  controller.save(controller_packet);

#ifdef SEND_TO_VESC
  send_to_vesc(controller.data.throttle, /*cruise*/ controller.data.cruise_control);
#endif

  board_packet.id = controller.data.id;
  board_packet.reason = ReasonType::RESPONSE;

  sendPacketToController();

  if (PRINT_THROTTLE && controller.throttleChanged())
  {
    DEBUGVAL(controller.data.id, controller.data.throttle);
  }
}
//------------------------------------------------------

void processConfigPacket()
{
  ControllerConfig _controller_config;

  uint8_t buff[sizeof(ControllerConfig)];
  nrf24.read_into(buff, sizeof(ControllerConfig));
  memcpy(&_controller_config, &buff, sizeof(ControllerConfig));

  board_packet.id = _controller_config.id;
  sendPacketToController();

  controller.save(_controller_config);

  DEBUGVAL("***config***", _controller_config.id, _controller_config.send_interval);
}
//------------------------------------------------------

bool sendPacketToController(ReasonType reason)
{
  board_packet.reason = reason;
  uint8_t buff[sizeof(VescData)];
  memcpy(&buff, &board_packet, sizeof(VescData));

#ifdef PRINT_SEND_TO_CONTROLLER
  DEBUGMVAL(getCDebugTime("[%6.1fs] sending:"), board_packet.id);
#endif

  uint8_t retryCount = 0;
  bool sent = false;
  while (!sent && retryCount < RETRY_COUNT_MAX)
  {
    sent = nrf24.send_packet(/*to*/ COMMS_CONTROLLER, 0, buff, sizeof(VescData));
    vTaskDelay(10);
    retryCount++;
  }

  if (!sent)
    Serial.printf("[%s] failed to send, retried %d times (max)\n", getCDebugTime(), retryCount);

  return sent;
}

//------------------------------------------------------

#ifndef FSM_H
#include <Fsm.h>
#endif

Fsm *commsFsm;

CommsStateEvent lastCommsEvent = EV_NONE;

State state_comms_offline([] {
  commsFsm->print("state_comms_offline", false);
  controller_connected = false;
});

State state_ctrlr_online([] {
  commsFsm->print("state_ctrlr_online");
  controller_connected = true;
});

State state_vesc_online([] {
  commsFsm->print("state_vesc_online");
});

State state_ctrlr_vesc_online([] {
  commsFsm->print("state_ctrlr_vesc_online");
});

//------------------------------------------------------
void addfsmCommsTransitions()
{
  // state_offline
  commsFsm->add_transition(&state_comms_offline, &state_ctrlr_online, EV_CTRLR_PKT, NULL);
  commsFsm->add_transition(&state_comms_offline, &state_vesc_online, EV_VESC_SUCCESS, NULL);

  // state_ctrlr_online
  commsFsm->add_transition(&state_ctrlr_online, &state_comms_offline, EV_CTRLR_TIMEOUT, NULL);
  commsFsm->add_transition(&state_ctrlr_online, &state_ctrlr_vesc_online, EV_VESC_SUCCESS, NULL);
  // state_vesc_online
  commsFsm->add_transition(&state_vesc_online, &state_ctrlr_vesc_online, EV_CTRLR_PKT, NULL);
  commsFsm->add_transition(&state_vesc_online, &state_comms_offline, EV_VESC_FAILED, NULL);
  // state_ctrlr_vesc_online
  commsFsm->add_transition(&state_ctrlr_vesc_online, &state_vesc_online, EV_CTRLR_TIMEOUT, NULL);
  commsFsm->add_transition(&state_ctrlr_vesc_online, &state_ctrlr_online, EV_VESC_FAILED, NULL);
}

//------------------------------------------------------
const char *commsEventToString(int ev)
{
  switch ((CommsStateEvent)ev)
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

void sendCommsStateEvent(CommsStateEvent ev)
{
  lastCommsEvent = ev;
  commsFsm->trigger(ev);

#ifdef PRINT_COMMS_STATE_EVENT

#ifdef SUPPRESS_EV_CTRLR_PKT
  return;
#endif
#ifdef SUPPRESS_EV_VESC_SUCCESS
  return;
#endif
#ifdef SUPPRESS_EV_CTRLR_TIMEOUT
  return;
#endif

  Serial.printf("-> COMMS_STATE EVENT -> %s\n", commsEventToString(ev));
#endif
}
//------------------------------------------------------