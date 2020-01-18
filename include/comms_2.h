void packet_available_cb(uint16_t from_id, uint8_t type)
{
  since_last_controller_packet = 0;

  uint8_t buff[sizeof(ControllerData)];
  nrf24.read_into(buff, sizeof(ControllerData));
  memcpy(&controller_packet, &buff, sizeof(ControllerData));

  NRF_EVENT(EV_NRF_PACKET, NULL);

  DEBUGVAL(from_id, controller_packet.id);
}

bool send_packet_to_controller(ReasonType reason)
{
  board_packet.reason = reason;

  uint8_t buff[sizeof(VescData)];
  memcpy(&buff, &board_packet, sizeof(VescData));
  
  uint8_t retries = nrf24.send_with_retries(/*to*/ COMMS_CONTROLLER, 0, buff, sizeof(VescData), 5);
  if (retries > 0)
  {
    DEBUGVAL(retries);
  }
  board_packet.id++;

  return retries < 5;
}