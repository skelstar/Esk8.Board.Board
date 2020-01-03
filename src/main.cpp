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

elapsedMillis sinceLastControllerPacket = 0;

bool sent_first_packet = false;
bool fake_vesc_online = false;
bool vescOnline = false;

void button_init();
void button_loop();

#define SECONDS 1000

// #define USE_TEST_VALUES
#ifdef USE_TEST_VALUES
#define CONTROLLER_TIMEOUT 1600
#define SEND_TO_VESC_INTERVAL 500 // times out after 1s
// #define SEND_TO_VESC
#else
#define CONTROLLER_TIMEOUT 200
#define SEND_TO_VESC_INTERVAL 500 // times out after 1s
#define SEND_TO_VESC
#endif


#define BUTTON_0 0
#define USING_BUTTONS 1
Button2 button0(BUTTON_0);

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

void controller_connected()
{
  TRIGGER(EV_CONTROLLER_CONNECTED);
}

void controller_disconnected()
{
  TRIGGER(EV_CONTROLLER_OFFLINE);
}

void packet_available_cb(uint16_t from_id)
{
  controller_id = from_id;

  int missed_packets = nrf24.controllerPacket.id - (old_packet.id + 1);
  if (missed_packets > 0)
  {
    DEBUGVAL("Missed packet from controller!", missed_packets);
  }
  memcpy(&old_packet, &nrf24.controllerPacket, sizeof(ControllerData));

  TRIGGER(EV_RECV_CONTROLLER_PACKET, NULL);

  bool request_update = nrf24.controllerPacket.command & COMMAND_REQUEST_UPDATE;
  if (request_update) 
  {
    send_to_packet_controller(ReasonType::REQUESTED);
  }
}

//------------------------------------------------------------------

xQueueHandle xEventQueue;
xQueueHandle xControllerTaskQueue;

SemaphoreHandle_t xVescDataSemaphore;

//------------------------------------------------------------------

Scheduler runner;

#define GET_FROM_VESC_INTERVAL 1000

Task t_GetVescValues(
    GET_FROM_VESC_INTERVAL,
    TASK_FOREVER,
    [] {
      if (xVescDataSemaphore != NULL && xSemaphoreTake(xVescDataSemaphore, (TickType_t)10) == pdTRUE)
      {
        vescOnline = getVescValues() == true;
        xSemaphoreGive(xVescDataSemaphore);
      }

      if (vescOnline == false && fake_vesc_online == false)
      {
        EventsEnum e = EV_VESC_OFFLINE;
        xQueueSendToFront(xEventQueue, &e, pdMS_TO_TICKS(10));
      }
      else
      {
        if (vescPoweringDown())
        {
          EventsEnum e = EV_POWERING_DOWN;
          xQueueSendToFront(xEventQueue, &e, pdMS_TO_TICKS(10));
        }
        else if (vescdata.moving)
        {
          EventsEnum e = EV_MOVING;
          xQueueSendToFront(xEventQueue, &e, pdMS_TO_TICKS(10));
        }
        else
        {
          EventsEnum e = EV_STOPPED;
          xQueueSendToFront(xEventQueue, &e, pdMS_TO_TICKS(10));
        }
      }
    });

Task t_SendToVesc(
    SEND_TO_VESC_INTERVAL,
    TASK_FOREVER,
    [] {
      #ifdef SEND_TO_VESC
      send_to_vesc(nrf24.controllerPacket.throttle);
      #endif
    });

#include "peripherals.h"

//------------------------------------------------------------------

#define OTHER_CORE 0
#define LOOP_CORE 1

void vescTask_0(void *pvParameters)
{
  Serial.printf("\nvescTask_0 running on core %d\n", xPortGetCoreID());

  vesc.init(VESC_UART_BAUDRATE);

  runner.startNow();
  runner.addTask(t_GetVescValues);
  runner.addTask(t_SendToVesc);

  t_GetVescValues.enable();
  t_SendToVesc.enable();

  while (true)
  {
    runner.execute();

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
    TRIGGER(EV_CONTROLLER_OFFLINE, "EV_CONTROLLER_OFFLINE");
  }
  // const uint8_t *peer_addr = peer.peer_addr;

  // vescdata.reason = reason;

  // uint8_t bs[sizeof(vescdata)];
  // memcpy(bs, &vescdata, sizeof(vescdata));
  // esp_err_t result = esp_now_send(peer_addr, bs, sizeof(bs));

  // if (result == ESP_OK)
  // {
  //   // DEBUGVAL(reason_toString(reason));
  // }
  // else 
  // {
  //   DEBUG("Failed sending to controller");
  // }
  // vescdata.id = vescdata.id + 1;
}
//----------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  Serial.println("Start");

  #ifdef USE_TEST_VALUES
  Serial.printf("\n");
  Serial.printf("/********************************************************/\n");
  Serial.printf("/*               WARNING: Using test values!            */\n");
  Serial.printf("/********************************************************/\n");
  Serial.printf("\n");
  #endif

  light_init();

  button_init();

  bool nrf_ok = nrf_setup();

  xTaskCreatePinnedToCore(vescTask_0, "vescTask", 10000, NULL, /*priority*/ 4, NULL, OTHER_CORE);
  xVescDataSemaphore = xSemaphoreCreateMutex();
  xEventQueue = xQueueCreate(1, sizeof(EventsEnum));

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

  // if (since_sent_to_controller > 3000)
  // {
  //   since_sent_to_controller = 0;
  //   send_to_packet_controller(ReasonType::REQUESTED);
  // }

  // if (sinceLastControllerPacket > CONTROLLER_TIMEOUT)
  // {
  //   TRIGGER(EV_CONTROLLER_OFFLINE);

    // ScanForPeer();
    // bool paired = pairPeer();
    // if (paired)
    // {
    //   sinceLastControllerPacket = 0;
    //   Serial.printf("Paired: %s\n", paired ? "true" : "false");

    //   // always send the first packet (id == 0)
    //   vescdata.id = 0;
    // }      
    // send_to_packet_controller(ReasonType::FIRST_PACKET);
  // }
}
//----------------------------------------------------------
