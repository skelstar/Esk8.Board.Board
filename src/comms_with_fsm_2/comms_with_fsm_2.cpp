// #define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>
#include <Smoothed.h>

#include <RF24Network.h>
#include <NRF24L01Lib.h>

#define SPI_CE 33
#define SPI_CS 26

#define COMMS_BOARD 00
#define COMMS_CONTROLLER 01

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

Smoothed<int> smoothed_throttle;

class BoardState
{
public:
  bool vesc_data_ready;
} state;

//------------------------------------------------------------------

#include <vesc_comms_2.h>
#include <comms_2.h>
#include <peripherals.h>

//-------------------------------------------------------
void setup()
{
  Serial.begin(115200);

#ifdef FEATURE_THROTTLE_SMOOTHING
  unsigned int smooth_factor = 2000 / 200;
  smoothed_throttle.begin(SMOOTHED_AVERAGE, smooth_factor);
#endif

  nrf24.begin(&radio, &network, COMMS_BOARD, packet_available_cb);
  vesc.init(VESC_UART_BAUDRATE);

  button_init();

  DEBUG("Ready to rx from controller...");
}

elapsedMillis since_sent_to_board, since_smoothed_report;

void loop()
{
  nrf24.update();

  button0.loop();

  vesc_update();

  if (controller_timed_out() && controller_connected)
  {
    controller_connected = false;
    DEBUG("controller_timed_out!!!");
  }

#ifdef PRINT_SMOOTHED_REPORT
#ifdef FEATURE_THROTTLE_SMOOTHING
  if (since_smoothed_report > 1000)
  {
    since_smoothed_report = 0;
    DEBUGVAL(smoothed_throttle.get());
  }
#endif
#endif

  vTaskDelay(10);
}