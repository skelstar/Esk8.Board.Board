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

uint16_t controller_id;

bool nrf_setup()
{
  nrf24.begin(&radio, &network, NRF_ADDRESS_SERVER, controller_packet_available_cb);
  return true;
}

void nrf_update()
{
  nrf24.update();
}  

void nrf_read(uint8_t *data, uint8_t data_len)
{
  nrf24.read_into(data, data_len);
}

bool nrf_send_to_controller()
{
  uint8_t bs[sizeof(VescData)];
  memcpy(bs, &board_packet, sizeof(VescData));
  return nrf24.sendPacket(controller_id, /*type*/0, bs, sizeof(VescData));
}