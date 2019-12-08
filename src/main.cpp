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

bool vescOnline = false;

void button_init();
void button_loop();
void initialiseApp();

#define CONTROLLER_TIMEOUT 600
#define SEND_TO_VESC_INTERVAL 900 // times out after 1s
#define MISSED_PACKET_COUNT_THAT_ZEROS_THROTTLE 3
#define SEND_TO_CONTROLLER_INTERVAL 1000

#define BUTTON_1 0
#define USING_BUTTONS true
Button2 btn1(BUTTON_1);

#define NUM_PIXELS 21
#define PIXEL_PIN 5
#define BRIGHT_MAX 10

// prototypes
void send_to_packet_controller();

#include "vesc_utils.h"
#include "utils.h"
#include "light-bar.h"
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
      send_to_vesc(controller_packet.throttle);
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
  bool missed_a_packet = controller_packet.id != old_packet.id + 1 && old_packet.id > 0;
  bool throttle_changed = controller_packet.throttle != old_packet.throttle;

  if (throttle_changed)
  {
    t_SendToVesc.restart();
  }

  if (missed_a_packet)
  {
    fsm.trigger(EV_MISSED_CONTROLLER_PACKET);
  }
  else 
  {
    fsm.trigger(EV_RECV_CONTROLLER_PACKET);
  }
}

void packetSent()
{
  // DEBUGFN("");
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

void manage_event_queue()
{
  BaseType_t xStatus;
  EventsEnum e;
  xStatus = xQueueReceive(xEventQueue, &e, pdMS_TO_TICKS(10));
  if (xStatus == pdPASS)
  {
    fsm.trigger(e);
  }
}
//----------------------------------------------------------

void send_to_packet_controller()
{
  const uint8_t *peer_addr = peer.peer_addr;

  uint8_t bs[sizeof(vescdata)];
  memcpy(bs, &vescdata, sizeof(vescdata));
  esp_err_t result = esp_now_send(peer_addr, bs, sizeof(bs));
  if (result == ESP_OK)
  {
    // DEBUGVAL("Sent to controller", vescdata.missing_packets);
  }
  else 
  {
    DEBUG("Failed sending to controller");
  }
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

  xTaskCreatePinnedToCore(vescTask_0, "vescTask", 10000, NULL, /*priority*/ 0, NULL, OTHER_CORE);
  xVescDataSemaphore = xSemaphoreCreateMutex();
  xEventQueue = xQueueCreate(1, sizeof(EventsEnum));

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

  button_loop();

  manage_event_queue();

  if (sinceSentToController > SEND_TO_CONTROLLER_INTERVAL)
  {
    if (!vescdata.moving)
    {
      send_to_packet_controller();
    }
    sinceSentToController = 0;
  }

  if (sinceLastControllerPacket > CONTROLLER_TIMEOUT)
  {
    fsm.trigger(EV_CONTROLLER_OFFLINE);

    // vescdata.missing_packets = 0;

    ScanForPeer();
    bool paired = pairPeer();
    if (paired)
    {
      sinceLastControllerPacket = 0;
      Serial.printf("Paired: %s\n", paired ? "true" : "false");

      vescdata.id = 0;

      send_to_packet_controller();
    }
  }
}
//----------------------------------------------------------
