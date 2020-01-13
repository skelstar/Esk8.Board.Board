#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Button2.h>
#include <Fsm.h>
#include <VescData.h>
#include <elapsedMillis.h>

// prototypes
void controller_packet_available_cb(uint16_t from_id, uint8_t type);

VescData vescdata, initialVescData, board_packet;

ControllerData controller_packet, old_packet;

#include "nrf.h"

elapsedMillis since_last_controller_packet = 0;

bool sent_first_packet = false;
bool vescOnline = false;

void button_init();
void button_loop();

#define SECONDS 1000

#ifdef USE_TEST_VALUES
#define CONTROLLER_TIMEOUT 1100
#define SEND_TO_VESC
#else
#define CONTROLLER_TIMEOUT 250
#define SEND_TO_VESC
#endif


#ifdef USING_BUTTONS

#define BUTTON_0 0
Button2 button0(BUTTON_0);

#endif

#define NUM_PIXELS  21
#define PIXEL_PIN   4
#define BRIGHT_MAX  10

// prototypes
void send_to_packet_controller(ReasonType reason);

#include "vesc_utils.h"
#include "utils.h"
#include <LedLightsLib.h>

LedLightsLib light;

#include "state_machine.h"

//------------------------------------------------------------------

xQueueHandle xEventQueue;
xQueueHandle xControllerTaskQueue;
xQueueHandle xSendToVescQueue;

SemaphoreHandle_t xVescDataSemaphore;

//------------------------------------------------------------------
/*
* checks timeout
* sends command to xSendToVescQueue
* if REQUEST then send_to_packet_controller
*/
void controller_packet_available_cb(uint16_t from_id, uint8_t type)
{
  controller_id = from_id;

  switch (type)
  {
    case 0:
      uint8_t buff[sizeof(ControllerData)];
      nrf_read(buff, sizeof(ControllerData));
      memcpy(&controller_packet, &buff, sizeof(ControllerData));
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
    TRIGGER(EV_CONTROLLER_OFFLINE);
    DEBUGVAL(since_last_controller_packet, CONTROLLER_TIMEOUT);
  }

  uint8_t e = 1;
  xQueueSendToFront(xSendToVescQueue, &e, pdMS_TO_TICKS(10));

  since_last_controller_packet = 0;

  int missed_packets = controller_packet.id - (old_packet.id + 1);
  if (missed_packets > 0 && old_packet.id > 0)
  {
    DEBUGVAL("Missed packet from controller!", missed_packets);
  }

  memcpy(&old_packet, &controller_packet, sizeof(ControllerData));

#ifdef DEBUG_THROTTLE_ENABLED
  DEBUGVAL(controller_packet.throttle, controller_packet.id);
#endif

  TRIGGER(EV_RECV_CONTROLLER_PACKET, NULL);

  bool request_update = controller_packet.command & COMMAND_REQUEST_UPDATE;
  if (request_update) 
  {
    send_to_packet_controller(ReasonType::REQUESTED);
  }
}

//------------------------------------------------------------------

void send_to_fsm_event_queue(EventsEnum e)
{
  xQueueSendToFront(xEventQueue, &e, pdMS_TO_TICKS(10));
}

#define GET_FROM_VESC_INTERVAL 1000

#ifdef FAKE_VESC_ONLINE
  bool fake_vesc_online = true;
#else
  bool fake_vesc_online = false;
#endif 

void try_get_values_from_vesc()
{
  if (xVescDataSemaphore != NULL && xSemaphoreTake(xVescDataSemaphore, (TickType_t)10) == pdTRUE)
  {
    vescOnline = getVescValues() == true;
    xSemaphoreGive(xVescDataSemaphore);
  }

  if (vescOnline == false && !fake_vesc_online)
  {
    send_to_fsm_event_queue(EV_VESC_OFFLINE);
  }
  else
  {
    if (vescPoweringDown())
    {
      send_to_fsm_event_queue(EV_POWERING_DOWN);
    }
    else if (vescdata.moving)
    {
      send_to_fsm_event_queue(EV_MOVING);
    }
    else
    {
      send_to_fsm_event_queue(EV_STOPPED);
    }
  }
}

#include "peripherals.h"

//------------------------------------------------------------------

#define OTHER_CORE 0
#define LOOP_CORE 

elapsedMillis since_got_values_from_vesc = 0;

void vescTask_0(void *pvParameters)
{
  Serial.printf("\nvescTask_0 running on core %d\n", xPortGetCoreID());

  #ifndef SEND_TO_VESC
  Serial.printf("*** NOT SENDING TO VESC\n");
  #endif

  vesc.init(VESC_UART_BAUDRATE);

  while (true)
  {
    // get values from vesc
    if (since_got_values_from_vesc > GET_FROM_VESC_INTERVAL)
    {
      since_got_values_from_vesc = 0;
      try_get_values_from_vesc();
    }

    // send to vesc
    uint8_t e;
    bool send_to_vesc_now = xQueueReceive(xSendToVescQueue, &e, pdMS_TO_TICKS(0)) == pdPASS;
    if (send_to_vesc_now)
    {
      #ifdef SEND_TO_VESC
      send_to_vesc(controller_packet.throttle);
      #endif
    }

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------------

void fsm_event_handler()
{
  EventsEnum e;
  bool event_ready = xQueueReceive(xEventQueue, &e, pdMS_TO_TICKS(0)) == pdPASS;

  if (event_ready)
  {
    TRIGGER(e);
  }
}
//----------------------------------------------------------

void send_to_packet_controller(ReasonType reason)
{
  board_packet.id++;
  board_packet.reason = reason;

  bool success = nrf_send_to_controller();

#ifdef DEBUG_SEND_TO_PACKET_CONTROLLER
    DEBUGVAL("Sent to controller", board_packet.id, reason_toString(reason), success);
#endif

}
//----------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  Serial.println("Start");

  bool nrf_ok = nrf_setup();

  #ifdef USE_TEST_VALUES
  Serial.printf("\n");
  Serial.printf("/********************************************************/\n");
  Serial.printf("/*               WARNING: Using test values!            */\n");
  Serial.printf("/********************************************************/\n");
  Serial.printf("\n");
  #endif

  #ifdef FAKE_VESC_ONLINE
  Serial.printf("\n");
  Serial.printf("/********************************************************/\n");
  Serial.printf("/*               WARNING: FAKE VESC ONLINE !            */\n");
  Serial.printf("/********************************************************/\n");
  Serial.printf("\n");
  #endif

  light_init();

  button_init();

  xTaskCreatePinnedToCore(vescTask_0, "vescTask", 10000, NULL, /*priority*/ 4, NULL, OTHER_CORE);
  xVescDataSemaphore = xSemaphoreCreateMutex();
  xEventQueue = xQueueCreate(1, sizeof(EventsEnum));
  xSendToVescQueue = xQueueCreate(1, sizeof(uint8_t));


  controller_packet.throttle = 127;

  fsm_add_transitions();
  fsm.run_machine();

#ifdef USING_BUTTONS
  button_init();
  button_loop();
#endif
}
//----------------------------------------------------------
elapsedMillis since_sent_to_controller = 0;

void loop()
{
  fsm.run_machine();

#ifdef USING_BUTTONS
  button_loop();
#endif

  fsm_event_handler();

  nrf_update();
}
//----------------------------------------------------------
