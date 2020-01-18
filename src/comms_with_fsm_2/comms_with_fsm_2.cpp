#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

#include <RF24Network.h>
#include <NRF24L01Lib.h>

#define SPI_CE 33
#define SPI_CS 26

#define COMMS_BOARD 00
#define COMMS_CONTROLLER  01

//------------------------------------------------------------------

VescData board_packet;

ControllerData controller_packet;

NRF24L01Lib nrf24;

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

#define NUM_RETRIES 5

//------------------------------------------------------------------

void send_to_controller();

//------------------------------------------------------------------

void packet_available_cb(uint16_t from_id, uint8_t type)
{
  uint8_t buff[sizeof(ControllerData)];
  nrf24.read_into(buff, sizeof(ControllerData));
  memcpy(&controller_packet, &buff, sizeof(ControllerData));

  if (controller_packet.command == 1)
  {
    board_packet.reason = ReasonType::REQUESTED;
    send_to_controller();
  }

  DEBUGVAL(from_id, controller_packet.id);
}

void send_to_controller()
{
  uint8_t buff[sizeof(VescData)];
  memcpy(&buff, &board_packet, sizeof(VescData));

  uint8_t retries = nrf24.send_with_retries(/*to*/ COMMS_CONTROLLER, 0, buff, sizeof(VescData), 5);
  if (retries > 0)
  {
    DEBUGVAL(retries);
  }
  board_packet.id++;
}

void setup()
{
  Serial.begin(115200);

  nrf24.begin(&radio, &network, COMMS_BOARD, packet_available_cb);

  DEBUG("Ready to rx from board...");
}

elapsedMillis since_sent_to_board;

void loop()
{
  nrf24.update();

  vTaskDelay(10);
}