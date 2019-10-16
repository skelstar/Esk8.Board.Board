#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <NRF24L01Library.h>

#define SPI_CE        33    	// white/purple
#define SPI_CS        26  	// green

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

NRF24L01Lib nrf24;

void packet_cb(uint16_t from);

void initialiseRF24Comms() {

  SPI.begin();
  radio.begin();
  radio.setAutoAck(true);
  nrf24.begin(&radio, &network, nrf24.RF24_SERVER, packet_cb);

  nrf24.boardPacket.id = 0;
}

void packet_cb(uint16_t from) {
  // Serial.printf("packet_cb(%d): %d\n", from, nrf24.controllerPacket.id);
}

void sendPacketToClient() {
  // nrf24.boardPacket.batteryVoltage = 123.23;

  if (nrf24.controllerPacket.id != nrf24.boardPacket.id) {
    Serial.printf("Controller didn't respond to %u (diff %d)\n", 
      nrf24.boardPacket.id, 
      nrf24.controllerPacket.id,
      nrf24.boardPacket.id - nrf24.controllerPacket.id);
  }

  nrf24.boardPacket.id++;

  bool success = nrf24.sendPacket(nrf24.RF24_CLIENT);
  if (success) {
    // Serial.printf("Sent OK\n");
  }
  else {
    Serial.printf("Failed to send\n");
  }
}
