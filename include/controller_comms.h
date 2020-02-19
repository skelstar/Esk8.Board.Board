
elapsedMillis since_requested;

void handle_request_command();
void handle_first_packet();
void handle_config_packet();
uint8_t manage_throttle(uint8_t controller_throttle);

#define NUM_RETRIES 5
#define FIRST_PACKET 0

/* prototypes */
uint8_t send_packet_to_controller(ReasonType reason);

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

    uint8_t retries = send_packet_to_controller(ReasonType::REQUESTED);
    if (retries)
    {
      DEBUGVAL(retries);
    }

#ifdef PRINT_THROTTLE
    if (old_throttle != controller_packet.throttle)
    {
      DEBUGVAL(controller_packet.throttle);
    }
#endif

    DEBUGVAL(controller_packet.id);

    // // COMMAND_REQUEST_UPDATE
    // if (controller_packet.command == COMMAND_REQUEST_UPDATE)
    // {
    //   handle_request_command();
    // }

    if (controller_packet.id == FIRST_PACKET)
    {
      handle_first_packet();
    }
  }
  else if (type == PacketType::CONFIG)
  {
    handle_config_packet();

    send_packet_to_controller(ReasonType::REQUESTED);
  }

  if (controller_config.send_interval >= 500)
  {
    DEBUGVAL(from_id, controller_packet.id, controller_packet.throttle);
  }
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
#ifdef PRINT_SENDING
  DEBUGVAL("sending", board_packet.id);
#endif
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
    DEBUGVAL("RESPONSE", retries);
  }
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
/*
if enabled
  get smoothed throttle. If not accelerating then don't smooth
else
  return bare throttle
*/
uint8_t manage_throttle(uint8_t controller_throttle)
{
#ifdef FEATURE_THROTTLE_SMOOTHING
  // if not accelerating then removing smoothing
  if (controller_throttle < smoothed_throttle.getLast())
  {
    smoothed_throttle.clear();
  }
  smoothed_throttle.add(controller_throttle);
  return smoothed_throttle.get();
#else
  return controller_throttle;
#endif
}
