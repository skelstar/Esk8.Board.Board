#include <RF24Network.h>
#include <NRF24L01Library.h>
#include <Arduino.h>

#define SPI_CE  33
#define SPI_CS  26

#define NRF_ADDRESS_SERVER    0
#define NRF_ADDRESS_CLIENT    1

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

NRF24L01Lib nrf24;

// prototypes
void read_board_packet();
bool send_to_packet_controller(ReasonType reason);

//------------------------------------------------------------------
/*
* checks timeout
* sends command to xSendToVescQueue
* if REQUEST then send_to_packet_controller
*/

elapsedMillis since_last_controller_packet = 0;

void controller_packet_available_cb(uint16_t from_id, uint8_t type)
{
  // DEBUGVAL("controller_packet_available_cb");

  switch (type)
  {
  case 0:
    read_board_packet();
    break;
  case 1:
    send_to_packet_controller(ReasonType::REQUESTED);
    break;
  default:
    DEBUGVAL("unhandled type", type);
    break;
  }

  if (since_last_controller_packet > CONTROLLER_TIMEOUT)
  {
    DEBUG("Sending 'test-online' packet");
    if (send_to_packet_controller(ReasonType::REQUESTED) == false)
    {
      DEBUG("unable to response to request (excessive retires)");
    }
    board_packet.id++;
  }
  since_last_controller_packet = 0;

  uint8_t e = 1;
  // xQueueSendToFront(xSendToVescQueue, &e, pdMS_TO_TICKS(10));

  // if (sent_first_packet == false)
  // {
  //   sent_first_packet = true;
  //   send_to_packet_controller(ReasonType::FIRST_PACKET);
  // }


  // int missed_packets = controller_packet.id - (old_packet.id + 1);
  // if (missed_packets > 0 && old_packet.id > 0)
  // {
  //   DEBUGVAL("Missed packet from controller!", missed_packets);
  // }

  memcpy(&old_packet, &controller_packet, sizeof(ControllerData));

#ifdef DEBUG_PRINT_THROTTLE_VALUE
  DEBUGVAL(controller_packet.throttle, controller_packet.id);
#endif

  bool request_update = controller_packet.command & COMMAND_REQUEST_UPDATE;
  if (request_update)
  {
    send_to_packet_controller(ReasonType::REQUESTED);
  }
}
//----------------------------------------------------------

#define NUM_RETRIES       5

void read_board_packet()
{
  uint8_t buff[sizeof(ControllerData)];
  nrf24.read_into(buff, sizeof(ControllerData));
  memcpy(&controller_packet, &buff, sizeof(ControllerData));
}

bool send_to_packet_controller(ReasonType reason)
{
  board_packet.reason = reason;
  uint8_t bs[sizeof(VescData)];
  memcpy(bs, &board_packet, sizeof(VescData));

  bool success = nrf24.send_with_retries(/*to*/1, 0, bs, sizeof(VescData), NUM_RETRIES);

#ifdef DEBUG_PRINT_SENT_TO_CONTROLLER
  DEBUGVAL("Sent to controller", board_packet.id, reason_toString(reason), success, retries);
#endif

  board_packet.id++;

  return success;
}
//----------------------------------------------------------
