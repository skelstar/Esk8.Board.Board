#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

#include <utils.h>

#ifdef USE_SPI2
#define SOFTSPI 1
#define SOFT_SPI_MISO_PIN 12 // Orange
#define SOFT_SPI_MOSI_PIN 13 // Blue
#define SOFT_SPI_SCK_PIN 15  // Yellow
#define SPI_CE 5             // 17
#define SPI_CS 2
#elif USING_M5STACK
#define SPI_CE 5
#define SPI_CS 13
#else
#define SPI_CE 33
#define SPI_CS 26
#endif

#ifndef PRINT_THROTTLE
#define PRINT_THROTTLE 0
#endif

#include <RF24Network.h>
#include <NRF24L01Lib.h>

#include <Fsm.h>

#define COMMS_BOARD 00
#define COMMS_CONTROLLER 01

//------------------------------------------------------------------

VescData board_packet;

ControllerClass controller;
NRF24L01Lib nrf24;

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

#define NUM_RETRIES 5

elapsedMillis
    sinceLastControllerPacket,
    sinceBoardBooted;
bool controller_connected = false;

//------------------------------------------------------------------

void send_to_vesc(uint8_t throttle, bool cruise_control);

#include <controller_comms.h>

#include <footLightTask_0.h>
#include <vesc_comms_2.h>
#include <peripherals.h>

//-------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  board_packet.version = VERSION;

  nrf24.begin(&radio, &network, COMMS_BOARD, packet_available_cb);
  vesc.init(VESC_UART_BAUDRATE);

  button_init();
  primaryButtonInit();

  //get chip id
  String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
  chipId.toUpperCase();

  print_build_status(chipId);

  if (boardIs(chipId, M5STACKFIREID))
  {
    DEBUG("-----------------------------------------------");
    DEBUG("               USING_M5STACK              ");
    DEBUG("-----------------------------------------------\n\n");
    m5StackButtons_init();
  }

  xTaskCreatePinnedToCore(
      footLightTask_0,
      "footLightTask_0",
      /*stack size*/ 10000,
      /*params*/ NULL,
      /*priority*/ 3,
      /*handle*/ NULL,
      /*core*/ 0);
  xFootLightEventQueue = xQueueCreate(1, sizeof(FootLightEvent));

  commsFsm = new Fsm(&state_comms_offline);
  commsFsm->setGetEventName(commsEventToString);
  addfsmCommsTransitions();

  // send startup packet
  board_packet.id = 0;
  sendPacketToController(FIRST_PACKET);
}

elapsedMillis sinceUpdatedButtonAValues;

void loop()
{
  nrf24.update();

  button0.loop();
  primaryButton.loop();
#ifdef USING_M5STACK
  buttonA.loop();
  if (sinceUpdatedButtonAValues > 1000 && buttonA.isPressed())
  {
    sinceUpdatedButtonAValues = 0;
    long r = random(300);
    board_packet.batteryVoltage -= r / 1000.0;
    if (MOCK_MOVING_WITH_BUTTON == 1)
      mockMoving(buttonA.isPressed());
  }

  buttonB.loop();
  buttonC.loop();
#endif

  vesc_update();

  commsFsm->run_machine();

  if (controller.hasTimedout(sinceLastControllerPacket))
  {
    // DEBUGMVAL("timeout", sinceLastControllerPacket);
    sendCommsStateEvent(EV_CTRLR_TIMEOUT);
  }

  vTaskDelay(10);
}