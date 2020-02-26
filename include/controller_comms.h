
elapsedMillis since_requested;

void handle_first_packet();
void handle_config_packet();
uint8_t manage_throttle(uint8_t controller_throttle);

#define NUM_RETRIES 5
#define FIRST_PACKET 0

/* prototypes */
bool send_packet_to_controller();

//------------------------------------------------------
void packet_available_cb(uint16_t from_id, uint8_t type)
{
  since_last_controller_packet = 0;
  controller_connected = true;
  uint8_t old_throttle = controller_packet.throttle;

  if (type == PacketType::CONTROL)
  {
    uint8_t buff[sizeof(ControllerData)];
    nrf24.read_into(buff, sizeof(ControllerData));
    memcpy(&controller_packet, &buff, sizeof(ControllerData));

    uint8_t throttle = manage_throttle(controller_packet.throttle);

#ifdef SEND_TO_VESC
    send_to_vesc(throttle, controller_packet.cruise_control);
#endif

    board_packet.id = controller_packet.id;
    bool success = send_packet_to_controller();
    if (false == success)
    {
      DEBUGVAL(success);
    }

#ifdef PRINT_THROTTLE
    if (old_throttle != controller_packet.throttle)
    {
      DEBUGVAL(controller_packet.id, controller_packet.throttle);
    }
#endif
  }
  else if (type == PacketType::CONFIG)
  {
    board_packet.id = controller_config.id;
    bool success = send_packet_to_controller();
    DEBUGVAL(controller_config.send_interval, success);

    handle_config_packet();
    DEBUGVAL(since_last_controller_packet);
  }
}
//------------------------------------------------------
bool send_packet_to_controller()
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
uint8_t manage_throttle(uint8_t controller_throttle)
{
  return controller_throttle;
}
