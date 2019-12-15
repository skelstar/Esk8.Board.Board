#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Button2.h>
#include <Fsm.h>
#include <TaskScheduler.h>
#include <VescData.h>
#include <espNowClient.h>
#include <elapsedMillis.h>

elapsedMillis sinceLastControllerPacket = 0;
elapsedMillis sinceSentToController = 0;

bool sent_first_packet = false;

bool vescOnline = false;

void button_init();
void button_loop();
void initialiseApp();

#define SECONDS 1000

#define CONTROLLER_TIMEOUT 600
#define SEND_TO_VESC_INTERVAL 500 // times out after 1s
#define MISSED_PACKET_COUNT_THAT_ZEROS_THROTTLE 3
#define SEND_TO_CONTROLLER_INTERVAL   10 * SECONDS
#define SEND_TO_VESC

#define BUTTON_1 0
#define USING_BUTTONS 0
Button2 button0(BUTTON_1);

#define NUM_PIXELS  21
#define PIXEL_PIN   4
#define BRIGHT_MAX  10

// prototypes
void send_to_packet_controller_1(ReasonType reason);

#include "vesc_utils.h"
#include "utils.h"
#include <LedLightsLib.h>

LedLightsLib light;

#include "state_machine.h"


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

      if (vescOnline == false)
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
      send_to_vesc(controller_packet.throttle);
      #endif
    });

#include "peripherals.h"

//------------------------------------------------------------------
void initialiseApp()
{
  fsm.trigger(EV_WAITING_FOR_VESC);
}
//------------------------------------------------------------------

void packetReceived(const uint8_t *data, uint8_t data_len)
{
  sinceLastControllerPacket = 0;

  memcpy(&old_packet, &controller_packet, sizeof(controller_packet));
  memcpy(/*dest*/ &controller_packet, /*src*/ data, data_len);

  bool throttle_changed = controller_packet.throttle != old_packet.throttle;
  bool request_update = controller_packet.command & COMMAND_REQUEST_UPDATE;

  fsm.trigger(EV_RECV_CONTROLLER_PACKET);

  if (throttle_changed)
  {
    DEBUGVAL(controller_packet.throttle);
    t_SendToVesc.restart();
  }

  if (sent_first_packet == false)
  {
    send_to_packet_controller_1(ReasonType::FIRST_PACKET);
    sent_first_packet = true;
  }

  if (request_update) 
  {
    send_to_packet_controller_1(ReasonType::REQUESTED);
  }
}

void packetSent()
{
}

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
    fsm.trigger(e);
  }
}
//----------------------------------------------------------

void send_to_packet_controller_1(ReasonType reason)
{
  const uint8_t *peer_addr = peer.peer_addr;

  vescdata.reason = reason;

  uint8_t bs[sizeof(vescdata)];
  memcpy(bs, &vescdata, sizeof(vescdata));
  esp_err_t result = esp_now_send(peer_addr, bs, sizeof(bs));

  if (result == ESP_OK)
  {
    DEBUGVAL(reason_toString(reason));
  }
  else 
  {
    DEBUG("Failed sending to controller");
  }
  vescdata.id = vescdata.id + 1;
}
//----------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  Serial.println("Start");

  initialiseApp();

  initialiseLeds();

  client.setOnConnectedEvent([] {
    Serial.printf("Connected!\n");
  });
  client.setOnDisconnectedEvent([] {
    Serial.println("ESPNow Init Failed, restarting...");
  });
  client.setOnNotifyEvent(packetReceived);
  client.setOnSentEvent(packetSent);
  initESPNow();

  button0.setPressedHandler([](Button2 &btn)
  {
    EventsEnum e = EV_MOVING;
    vescdata.moving = true;
    xQueueSendToFront(xEventQueue, &e, pdMS_TO_TICKS(10));
  });
  button0.setReleasedHandler([](Button2 &btn)
  {
    EventsEnum e = EV_STOPPED;
    vescdata.odometer = vescdata.odometer + 0.1;
    vescdata.moving = false;
    xQueueSendToFront(xEventQueue, &e, pdMS_TO_TICKS(10));
  });

  xTaskCreatePinnedToCore(vescTask_0, "vescTask", 10000, NULL, /*priority*/ 4, NULL, OTHER_CORE);
  xVescDataSemaphore = xSemaphoreCreateMutex();
  xEventQueue = xQueueCreate(1, sizeof(EventsEnum));

  controller_packet.throttle = 127;

  light.initialise(PIXEL_PIN, NUM_PIXELS);
  light.setAll(light.COLOUR_OFF);

  addFsmTransitions();
  fsm.run_machine();

#ifdef USING_BUTTONS
  button_init();
  button_loop();
#endif
}
//----------------------------------------------------------
unsigned long now = 0;

void loop()
{
  fsm.run_machine();

#ifdef USING_BUTTONS
  button_loop();
#endif

  manage_xEventQueue_1();

  if (sinceLastControllerPacket > CONTROLLER_TIMEOUT)
  {
    fsm.trigger(EV_CONTROLLER_OFFLINE);

    ScanForPeer();
    bool paired = pairPeer();
    if (paired)
    {
      sinceLastControllerPacket = 0;
      Serial.printf("Paired: %s\n", paired ? "true" : "false");

      vescdata.id = 0;

      // always send the first packet (id == 0)
    }      
    send_to_packet_controller_1(ReasonType::FIRST_PACKET);
  }
}
//----------------------------------------------------------
