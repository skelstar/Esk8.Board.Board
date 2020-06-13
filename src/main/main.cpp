#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>
#include <Smoothed.h>

#ifdef USE_SPI2
#define SOFTSPI 1
#define SOFT_SPI_MOSI_PIN 13 // Blue
#define SOFT_SPI_MISO_PIN 12 // Orange
#define SOFT_SPI_SCK_PIN 15  // Yellow
#define SPI_CE 5             // 17
#define SPI_CS 2
#else
#define SPI_CE 33
#define SPI_CS 26
#endif

#include <RF24Network.h>
#include <NRF24L01Lib.h>

#include <Fsm.h>

#define COMMS_BOARD 00
#define COMMS_CONTROLLER 01

//------------------------------------------------------------------

VescData board_packet;

ControllerData controller_packet, prevControllerPacket;
ControllerConfig controller_config;

NRF24L01Lib nrf24;

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

#define NUM_RETRIES 5

elapsedMillis since_last_controller_packet;
bool controller_connected = false;

Smoothed<int> smoothed_throttle;

//------------------------------------------------------------------

void send_to_vesc(uint8_t throttle, bool cruise_control);

#include <controller_comms.h>
#include <utils.h>

#include <footLightTask_0.h>
#include <vesc_comms_2.h>
#include <peripherals.h>

//-------------------------------------------------------
void setup()
{
  Serial.begin(115200);

  nrf24.begin(&radio, &network, COMMS_BOARD, packet_available_cb);
  vesc.init(VESC_UART_BAUDRATE);

  button_init();

  xTaskCreatePinnedToCore(footLightTask_0, "footLightTask_0", 10000, NULL, /*priority*/ 3, NULL, 0);

  xFootLightEventQueue = xQueueCreate(1, sizeof(FootLightEvent));

  addCommsFsmTransitions();

  // send startup packet
  board_packet.id = 0;
  board_packet.reason = ReasonType::FIRST_PACKET;
  sendPacketToController();
}

elapsedMillis since_smoothed_report, since;

void loop()
{
  nrf24.update();

  button0.loop();

  vesc_update();

  commsFsm.run_machine();

  if (controller_timed_out())
  {
    sendCommsStateEvent(EV_CTRLR_TIMEOUT);
  }

  vTaskDelay(10);
}