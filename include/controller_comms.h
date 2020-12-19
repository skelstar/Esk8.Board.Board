uint8_t manage_throttle(uint8_t controller_throttle);

elapsedMillis since_requested;

/* prototypes */
bool sendPacketToController(ReasonType reason = RESPONSE);
void processControlPacket();
void processConfigPacket();

//------------------------------------------------------
void controllerPacketAvailable_cb(uint16_t from_id, uint8_t type)
{
  sinceLastControllerPacket = 0;

  if (type == Packet::CONTROL)
  {
    ControllerData data = controllerClient.read();
    controller.save(data);

    if (SEND_TO_VESC)
      send_to_vesc(controller.data.throttle, /*cruise*/ controller.data.cruise_control);

    board_packet.id = controller.data.id;
    board_packet.reason = ReasonType::RESPONSE;

    sendPacketToController();

    if (PRINT_THROTTLE && controller.throttleChanged())
    {
      DEBUGVAL(controller.data.id, controller.data.throttle);
    }
  }
  else if (type == Packet::CONFIG)
  {
    ControllerConfig config = controllerClient.readAlt<ControllerConfig>();
    controller.save(config);

    board_packet.id = controller.config.id;

    sendPacketToController();

    DEBUGVAL("***config***", controller.config.id, controller.config.send_interval);
  }
  else
  {
    Serial.printf("unknown packet from controller: %d\n", type);
  }
}

//------------------------------------------------------

bool sendPacketToController(ReasonType reason)
{
  board_packet.id = 0;
  board_packet.reason = reason;

  bool sent = controllerClient.sendTo(Packet::CONTROL, board_packet);

  // TODO: tidy this up
  if (board_packet.command == CommandType::RESET)
  {
    Serial.printf("sent command=RESET\n");
  }
  board_packet.command = NONE;

  return sent;
}

//------------------------------------------------------

#ifndef FSM_H
#include <Fsm.h>
#endif

namespace COMMS
{
  Event lastCommsEvent = EV_NONE;

  FsmManager<COMMS::Event> commsFsm;

  State stateOffline([] {
    commsFsm.printState(OFFLINE);
    controller_connected = false;
  });

  State stateCtrlrOnline([] {
    commsFsm.printState(CTRLR_ONLINE);
    controller_connected = true;
  });

  State stateVescOnline([] {
    commsFsm.printState(VESC_ONLINE);
  });

  State stateCtrlrVescOnline([] {
    commsFsm.printState(CTRLR_VESC_ONLINE);
  });

  Fsm fsm(&stateOffline);

  //------------------------------------------------------
  void addTransitions()
  {
    // state_offline
    fsm.add_transition(&stateOffline, &stateCtrlrOnline, EV_CTRLR_PKT, NULL);
    fsm.add_transition(&stateOffline, &stateVescOnline, EV_VESC_SUCCESS, NULL);

    // stateCtrlrOnline
    fsm.add_transition(&stateCtrlrOnline, &stateOffline, EV_CTRLR_TIMEOUT, NULL);
    fsm.add_transition(&stateCtrlrOnline, &stateCtrlrVescOnline, EV_VESC_SUCCESS, NULL);
    // stateVescOnline
    fsm.add_transition(&stateVescOnline, &stateCtrlrVescOnline, EV_CTRLR_PKT, NULL);
    fsm.add_transition(&stateVescOnline, &stateOffline, EV_VESC_FAILED, NULL);
    // stateCtrlrVescOnline
    fsm.add_transition(&stateCtrlrVescOnline, &stateVescOnline, EV_CTRLR_TIMEOUT, NULL);
    fsm.add_transition(&stateCtrlrVescOnline, &stateCtrlrOnline, EV_VESC_FAILED, NULL);
  }
} // namespace COMMS