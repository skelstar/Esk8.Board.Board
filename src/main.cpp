#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Button2.h>
#include <Fsm.h>
#include <TaskScheduler.h>
#include <VescData.h>
#include <elapsedMillis.h>

// prototypes
void packet_available_cb(uint16_t from_id);

#include "nrf.h"

VescData vescdata, initialVescData;

ControllerData old_packet;

elapsedMillis since_last_controller_packet = 0;

bool sent_first_packet = false;
bool vescOnline = false;

void button_init();
void button_loop();

#define SECONDS 1000

#ifdef USE_TEST_VALUES
#define CONTROLLER_TIMEOUT 1600
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

uint16_t missed_packets_accumulated = 0;

#include "state_machine.h"

//------------------------------------------------------------------

xQueueHandle xEventQueue;
xQueueHandle xControllerTaskQueue;
xQueueHandle xSendToVescQueue;

SemaphoreHandle_t xVescDataSemaphore;

//------------------------------------------------------------------

void packet_available_cb(uint16_t from_id)
{
  controller_id = from_id;
  since_last_controller_packet = 0;

  int missed_packets = nrf24.controllerPacket.id - (old_packet.id + 1);
  if (missed_packets > 0 && old_packet.id > 0)
  {
    missed_packets_accumulated += missed_packets;
    nrf24.boardPacket.ampHours = (float)missed_packets_accumulated;
    DEBUGVAL("Missed packet from controller!", missed_packets, missed_packets_accumulated);
  }

  memcpy(&old_packet, &nrf24.controllerPacket, sizeof(ControllerData));

  uint8_t e = 1;
  xQueueSendToFront(xSendToVescQueue, &e, pdMS_TO_TICKS(10));

#ifdef DEBUG_THROTTLE_ENABLED
  DEBUGVAL(nrf24.controllerPacket.throttle);
#endif

  TRIGGER(EV_RECV_CONTROLLER_PACKET, NULL);

  bool request_update = nrf24.controllerPacket.command & COMMAND_REQUEST_UPDATE;
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

Scheduler runner;

#define GET_FROM_VESC_INTERVAL 1000

#ifdef FAKE_VESC_ONLINE
  bool fake_vesc_online = true;
#else
  bool fake_vesc_online = false;
#endif 

Task t_GetVescValues_0(
    GET_FROM_VESC_INTERVAL,
    TASK_FOREVER,
    [] {
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
    });

#include "peripherals.h"

//------------------------------------------------------------------

#define OTHER_CORE 0
#define LOOP_CORE 1

void vescTask_0(void *pvParameters)
{
  Serial.printf("\nvescTask_0 running on core %d\n", xPortGetCoreID());

  #ifndef SEND_TO_VESC
  Serial.printf("*** NOT SENDING TO VESC\n");
  #endif

  vesc.init(VESC_UART_BAUDRATE);

  runner.startNow();
  runner.addTask(t_GetVescValues_0);

  t_GetVescValues_0.enable();

  while (true)
  {
    runner.execute();

    BaseType_t xStatus;
    uint8_t e;
    xStatus = xQueueReceive(xSendToVescQueue, &e, pdMS_TO_TICKS(0));
    if (xStatus == pdPASS)
    {
      #ifdef SEND_TO_VESC
      send_to_vesc(nrf24.controllerPacket.throttle);
      #endif
    }

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------------

void manage_xEventQueue_1()
{
  BaseType_t xStatus;
  EventsEnum e;
  xStatus = xQueueReceive(xEventQueue, &e, pdMS_TO_TICKS(0));
  if (xStatus == pdPASS)
  {
    TRIGGER(e);
  }
}
//----------------------------------------------------------

void send_to_packet_controller(ReasonType reason)
{
  // send last controllerPacket.id as boardPacket.id
  nrf24.boardPacket.id = nrf24.controllerPacket.id;
  nrf24.boardPacket.reason = reason;

  bool success = nrf_send_to_controller();
  if (success)
  {
    DEBUGVAL("Sent to controller", nrf24.boardPacket.id);
  }
  else 
  {
    TRIGGER(EV_CONTROLLER_OFFLINE, "EV_CONTROLLER_OFFLINE: Couldn't send to controller");
  }
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


  nrf24.controllerPacket.throttle = 127;

  addFsmTransitions();
  fsm.run_machine();

#ifdef USING_BUTTONS
  button_init();
  button_loop();
#endif
}
//----------------------------------------------------------
unsigned long now = 0;
elapsedMillis since_sent_to_controller = 0;

void loop()
{
  fsm.run_machine();

#ifdef USING_BUTTONS
  button_loop();
#endif

  manage_xEventQueue_1();

  nrf24.update();

  if (since_last_controller_packet > CONTROLLER_TIMEOUT)
  {
    TRIGGER(EV_CONTROLLER_OFFLINE);
  }
}
//----------------------------------------------------------
