
elapsedMillis since_requested;

void handle_request_command();
void handle_first_packet();
void handle_config_packet();

#define NUM_RETRIES   5
#define FIRST_PACKET  0

//------------------------------------------------------
void packet_available_cb(uint16_t from_id, uint8_t type)
{
  since_last_controller_packet = 0;
  controller_connected = true;

  if (type == PacketType::CONTROL)
  {
    uint8_t buff[sizeof(ControllerData)];
    nrf24.read_into(buff, sizeof(ControllerData));
    memcpy(&controller_packet, &buff, sizeof(ControllerData));

    // COMMAND_REQUEST_UPDATE
    if (controller_packet.command == COMMAND_REQUEST_UPDATE)
    {
      handle_request_command();
    }
    else if (controller_packet.id == FIRST_PACKET)
    {
      handle_first_packet();
    }
  }
  else if (type == PacketType::CONFIG)
  {
    handle_config_packet();
  }

  DEBUGVAL(from_id, controller_packet.id, controller_packet.throttle);
}
//------------------------------------------------------
uint8_t send_packet_to_controller(ReasonType reason)
{
  board_packet.reason = reason;

  uint8_t buff[sizeof(VescData)];
  memcpy(&buff, &board_packet, sizeof(VescData));
  
  uint8_t retries = nrf24.send_with_retries(/*to*/ COMMS_CONTROLLER, 0, buff, sizeof(VescData), NUM_RETRIES);
  if (retries > 0)
  {
    DEBUGVAL(retries);
  }
  DEBUGVAL("sending", board_packet.id);
  board_packet.id++;

  return retries;
}
//------------------------------------------------------
void handle_request_command()
{
    since_requested = 0;
    controller_packet.command = 0;
    uint8_t retries = send_packet_to_controller(ReasonType::REQUESTED);
    if (retries > 0)
    {
      DEBUGVAL(retries);
    }
    DEBUGVAL("sent response", board_packet.id);
}
//------------------------------------------------------
void handle_first_packet()
{
  DEBUG("handle_first_packet();");
}
//------------------------------------------------------
void handle_config_packet()
{
  uint8_t buff[sizeof(ControllerConfig)];
  nrf24.read_into(buff, sizeof(ControllerConfig));
  memcpy(&controller_config, &buff, sizeof(ControllerConfig));
  DEBUGVAL("***controller_config***", controller_config.send_interval);
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