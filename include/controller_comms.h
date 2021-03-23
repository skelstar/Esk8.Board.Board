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

    // should only do this if we have something subscribed to it
    ctrlrQueue->send(&controller);

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