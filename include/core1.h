#include <RF24Network.h>
#include <NRF24L01Library.h>
#include <Arduino.h>

#define SPI_CE 33
#define SPI_CS 26

#define NRF_ADDRESS_SERVER 0
#define NRF_ADDRESS_CLIENT 1

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

NRF24L01Lib nrf24;

// prototypes
void read_board_packet();
bool send_to_packet_controller(ReasonType reason);
void send_to_vesc_queue();

//------------------------------------------------------------------
/*
* sends command to xSendToVescQueue
*/

void controller_packet_available_cb(uint16_t from_id, uint8_t type)
{
  if (since_last_controller_packet > CONTROLLER_TIMEOUT)
  {
    DEBUGVAL("controller_packet_available_cb TIMEOUT", since_last_controller_packet);
  }
  since_last_controller_packet = 0;
  read_board_packet();

  send_to_vesc_queue();
  
  NRF_EVENT(EV_NRF_PACKET, NULL);
  nrf_fsm.run_machine();

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

#define NUM_RETRIES 5

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

  bool success = nrf24.send_with_retries(/*to*/ 1, 0, bs, sizeof(VescData), NUM_RETRIES);

#ifdef DEBUG_PRINT_SENT_TO_CONTROLLER
  DEBUGVAL("Sent to controller", board_packet.id, reason_toString(reason), success, retries);
#endif

  board_packet.id++;

  return success;
}
//----------------------------------------------------------

void commsTask_1(void *pvParameters)
{
  while (true)
  {
    nrf24.update();

    vTaskDelay(1);
  }
  vTaskDelete(NULL);
}
//----------------------------------------------------------

void fsmTask_1(void *pvParameters)
{
  while (true)
  {
    nrf_fsm.run_machine();

    nrf24.update();

    vTaskDelay(1);
  }
  vTaskDelete(NULL);
}
//----------------------------------------------------------

void buttonTask_1(void *pvParameters)
{
#define BUTTON_CHECK_INTERVAL 500

  Serial.printf("\buttonTask_1 running on core %d\n", xPortGetCoreID());
  elapsedMillis since_checked_button = 0;

#ifdef USING_BUTTONS
  button_init();
#endif
  while (true)
  {
#ifdef USING_BUTTONS
    if (since_checked_button > BUTTON_CHECK_INTERVAL)
    {
      since_checked_button = 0;
      button0.loop();
    }
#endif

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//-----------------------------------------------------------------------
void send_to_vesc_queue()
{
  uint8_t e = 1;
  if (xSendToVescQueue != NULL)
  {
    xQueueSendToFront(xSendToVescQueue, &e, pdMS_TO_TICKS(10));
  }
}
//-----------------------------------------------------------------------
