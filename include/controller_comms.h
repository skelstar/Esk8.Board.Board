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

  Comms::commsFsm.trigger(Comms::EV_CTRLR_PKT);

  if (type == Packet::CONTROL)
  {
    ControllerData data = controllerClient.read();
    controller.save(data);

    if (SEND_TO_VESC)
      send_to_vesc(controller.data.throttle, /*cruise*/ controller.data.cruise_control);

#if USING_M5STACK
    if (controller.throttleChanged())
    {
      if (controller.data.throttle == 127)
      {
        displayQueue->send(M5StackDisplay::Q_STOPPED);
      }
      else if (controller.data.throttle > 127)
      {
        displayQueue->send(M5StackDisplay::Q_RL_MOVING);
      }
      else if (controller.data.throttle < 127)
      {
        displayQueue->send(M5StackDisplay::Q_RL_STOPPING);
      }
    }
#endif

    board_packet.id = controller.data.id;
    board_packet.reason = ReasonType::RESPONSE;

    sendPacketToController();

    if (PRINT_THROTTLE && controller.throttleChanged())
    {
      DEBUGVAL(controller.data.id, controller.data.throttle, controller.data.cruise_control);
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

namespace Comms
{
  Event lastCommsEvent = EV_NONE;

  State stOffline(
      [] {
        commsFsm.printState(OFFLINE);
        controller_connected = false;
      },
      NULL, NULL);

  State stCtrlrOnline(
      [] {
        commsFsm.printState(CTRLR_ONLINE);
        controller_connected = true;
      },
      NULL, NULL);

  State stVescOnline(
      [] {
        commsFsm.printState(VESC_ONLINE);
      },
      NULL, NULL);

  State stCtrlrVescOnline(
      [] {
        commsFsm.printState(CTRLR_VESC_ONLINE);
      },
      NULL, NULL);

  Fsm fsm(&stOffline);

  //------------------------------------------------------
  void addTransitions()
  {
    // state_offline
    fsm.add_transition(&stOffline, &stCtrlrOnline, EV_CTRLR_PKT, NULL);
    fsm.add_transition(&stOffline, &stVescOnline, EV_VESC_SUCCESS, NULL);

    // stCtrlrOnline
    fsm.add_transition(&stCtrlrOnline, &stOffline, EV_CTRLR_TIMEOUT, NULL);
    fsm.add_transition(&stCtrlrOnline, &stCtrlrVescOnline, EV_VESC_SUCCESS, NULL);

    // stVescOnline
    fsm.add_transition(&stVescOnline, &stCtrlrVescOnline, EV_CTRLR_PKT, NULL);
    fsm.add_transition(&stVescOnline, &stOffline, EV_VESC_FAILED, NULL);

    // stCtrlrVescOnline
    fsm.add_transition(&stCtrlrVescOnline, &stVescOnline, EV_CTRLR_TIMEOUT, NULL);
    fsm.add_transition(&stCtrlrVescOnline, &stCtrlrOnline, EV_VESC_FAILED, NULL);
  }
} // namespace Comms