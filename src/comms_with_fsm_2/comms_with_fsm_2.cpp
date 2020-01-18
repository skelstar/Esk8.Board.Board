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
ControllerConfig controller_config;

NRF24L01Lib nrf24;

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

#define NUM_RETRIES 5

elapsedMillis since_last_controller_packet;
bool controller_connected = true;

//------------------------------------------------------------------

#include <comms_2.h>

//-------------------------------------------------------
void setup()
{
  Serial.begin(115200);

  nrf24.begin(&radio, &network, COMMS_BOARD, packet_available_cb);

  DEBUG("Ready to rx from controller...");
}

elapsedMillis since_sent_to_board;

void loop()
{
  nrf24.update();

  if (controller_timed_out() && controller_connected)
  {
    controller_connected = false;
    DEBUG("controller_timed_out!!!");
  }

  vTaskDelay(10);
}