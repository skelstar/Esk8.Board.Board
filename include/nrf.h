#include <RF24Network.h>
#include <NRF24L01Library.h>
#include <Arduino.h>

#define SPI_CE  33
#define SPI_CS  26

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

NRF24L01Lib nrf24;

uint16_t controller_id;

bool nrf_setup()
{
  nrf24.begin(&radio, &network, nrf24.RF24_SERVER, packet_available_cb);
  return true;
}


bool nrf_send_to_controller()
{
  return nrf24.sendPacket(controller_id);
}