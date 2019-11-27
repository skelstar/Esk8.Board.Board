#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Button2.h>
#include <Fsm.h>
#include <TaskScheduler.h>
#include <VescData.h>

#define ADC_EN 14
#define ADC_PIN 34
#define BUTTON_1 0

volatile bool responded = true;
bool vescOnline = false;

void button_init();
void button_loop();
void initialiseApp();

#define USING_BUTTONS true
Button2 btn1(BUTTON_1);

#define NUM_PIXELS  21
#define PIXEL_PIN   5
#define BRIGHT_MAX  10

#include "vesc_utils.h"
#include "utils.h"
#include "light-bar.h"

#include "ESPNow.h"

//------------------------------------------------------------------

enum EventsEnum
{
  WAITING_FOR_VESC,
  POWERING_DOWN,
  VESC_OFFLINE,
  MOVING,
  STOPPED
} event;
//------------------------------------------------------------------
State state_waiting_for_vesc([] {
  Serial.printf("state_waiting_for_vesc\n");
},
NULL, NULL);
//------------------------------------------------------------------
State state_powering_down([] {
  Serial.printf("state_powering_down\n");
},
NULL, NULL);
//------------------------------------------------------------------
State state_offline([] {
  Serial.printf("state_offline\n");
},
NULL, NULL);
//------------------------------------------------------------------
State state_board_moving([] {
  Serial.printf("state_board_moving\n");
},
NULL, NULL);
//------------------------------------------------------------------
State state_board_stopped([] {
  Serial.printf("state_board_stopped\n");
  if (vescOnline)
  {
    // lastStableVolts = vescdata.batteryVoltage;
  }
},
NULL, NULL);
//------------------------------------------------------------------
Fsm fsm(&state_waiting_for_vesc);

void addFsmTransitions()
{
  event = POWERING_DOWN;
  fsm.add_transition(&state_waiting_for_vesc, &state_powering_down, event, NULL);
  fsm.add_transition(&state_board_moving, &state_powering_down, event, NULL);
  fsm.add_transition(&state_board_stopped, &state_powering_down, event, NULL);

  event = VESC_OFFLINE;
  fsm.add_transition(&state_board_moving, &state_offline, event, NULL);
  fsm.add_transition(&state_board_stopped, &state_offline, event, NULL);

  event = MOVING;
  fsm.add_transition(&state_board_stopped, &state_board_moving, event, NULL);

  event = STOPPED;
  fsm.add_transition(&state_waiting_for_vesc, &state_board_stopped, event, NULL);
  fsm.add_transition(&state_board_moving, &state_board_stopped, event, NULL);
}
//------------------------------------------------------------------
bool fakeVescData = false;
bool fakeMoving = false;

void toggleFakeVescMode() {
  fakeVescData = !fakeVescData;
  fakeMoving = false;
}

//------------------------------------------------------------------

Scheduler runner;

#define GET_FROM_VESC_INTERVAL 1000

Task t_GetVescValues(
    GET_FROM_VESC_INTERVAL,
    TASK_FOREVER,
    [] {
      vescOnline = getVescValues() == true;

      if (fakeVescData) {
        vescOnline = true;
        vescdata.ampHours = vescdata.ampHours > 0.0 ? vescdata.ampHours + 0.23 : 12.0;
        vescdata.odometer = vescdata.odometer > 0.0 ? vescdata.odometer + 0.1 : 1.0;
        vescdata.batteryVoltage = 38.4;
        vescdata.moving = fakeMoving;
        vescdata.vescOnline = true;
        Serial.printf("FAKE: ampHours %.1fmAh odo %.1fkm batt: %.1fv \n", vescdata.ampHours, vescdata.odometer, vescdata.batteryVoltage);
      }

      if (vescOnline == false)
      {
        fsm.trigger(VESC_OFFLINE);
      }
      else
      {
        if (vescPoweringDown())
        {
          fsm.trigger(POWERING_DOWN);
        }
        else if (vescdata.moving)
        {
          fsm.trigger(MOVING);
        }
        else
        {
          fsm.trigger(STOPPED);
          fsm.run_machine();
        }
      }

      if (clientConnected)
      {
        // sendDataToClient();
      }

      fsm.run_machine();
    });

//------------------------------------------------------------------

#define LONGCLICK_MS    1000

void button_init()
{
  btn1.setPressedHandler([](Button2 &b) {
    Serial.printf("btn1.setPressedHandler()\n");
  });
  btn1.setReleasedHandler([](Button2 &b) {
    Serial.printf("btn1.setReleasedHandler()\n");
  });
  btn1.setClickHandler([](Button2 &b) {
    if (fakeVescData) {
      fakeMoving = !fakeMoving;
    }
    Serial.printf("btn1.setClickHandler()\n");
  });
  btn1.setLongClickHandler([](Button2 &b) {
    toggleFakeVescMode();
    Serial.printf("Faking vesc: %d\n", fakeVescData);
    // Serial.printf("btn1.setLongClickHandler([](Button2 &b)\n");
  });
  btn1.setDoubleClickHandler([](Button2 &b) {
    Serial.printf("btn1.setDoubleClickHandler([](Button2 &b)\n");
  });
  btn1.setTripleClickHandler([](Button2 &b) {
    Serial.printf("btn1.setTripleClickHandler([](Button2 &b)\n");
  });
}

void button_loop()
{
  btn1.loop();
}
//------------------------------------------------------------------
void initialiseApp()
{
  fsm.trigger(WAITING_FOR_VESC);
}
//------------------------------------------------------------------
void initialiseLeds() {
  Serial.printf("Initialising LEDs (red)\n");
  FastLED.addLeds<WS2812B, PIXEL_PIN, GRB>(strip, NUM_PIXELS);
  FastLED.setBrightness(50);
  allLedsOn(COLOUR_RED);
  FastLED.show();
}
//------------------------------------------------------------------

xQueueHandle xVescTaskQueue;
xQueueHandle xControllerTaskQueue;
const TickType_t xVescTicksToWait = pdMS_TO_TICKS(100);
const TickType_t xControllerTicksToWait = pdMS_TO_TICKS(100);

#define OTHER_CORE  0
#define LOOP_CORE   1

void vescTask(void *pvParameters)
{

  Serial.printf("vescTask running on core %d\n", xPortGetCoreID());

  while (true)
  {
    vTaskDelay(10);
    runner.execute();
  }
  vTaskDelete(NULL);
}

void controllerTask(void *pvParameters)
{
  Serial.printf("controllerTask running on core %d\n", xPortGetCoreID());

  while (true)
  {
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

//----------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  Serial.println("Start");

  initialiseApp();

  initialiseLeds();

  setupESPNow();
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  vesc.init(VESC_UART_BAUDRATE);

  runner.startNow();
  runner.addTask(t_GetVescValues);
  t_GetVescValues.enable();
  
  xTaskCreatePinnedToCore(vescTask, "vescTask", 10000, NULL, /*priority*/ 0, NULL, OTHER_CORE);
  xTaskCreatePinnedToCore(controllerTask, "controllerTask", 10000, NULL, /*priority*/ 1, NULL, LOOP_CORE);

  addFsmTransitions();
  fsm.run_machine();

  #ifdef USING_BUTTONS
  button_init();
  button_loop();
  #endif
  //waitForFirstPacketFromVesc();
}
//----------------------------------------------------------
unsigned long now = 0;

void loop()
{
  fsm.run_machine();

  button_loop();

  if (millis() - now > 1000)
  {
    now = millis();

    if (slave.channel == CHANNEL && millis() - lastPacketRxTime < 1000)
    {
      bool exists = esp_now_is_peer_exist(slave.peer_addr);
      if (exists)
      {
        // sendData();
      }
      else
      {
        Serial.println("Slave pair failed!");
      }
    }
    else if (clientConnected == false || millis() - lastPacketRxTime > 1000)
    {
      ScanForSlave();
      bool paired = pairSlave();
      if (paired)
      {
        Serial.printf("Paired: %s\n", paired ? "true" : "false");
        
        VescData vescdata;
        vescdata.id = 0;

        const uint8_t *peer_addr = slave.peer_addr;

        uint8_t bs[sizeof(vescdata)];
        memcpy(bs, &vescdata, sizeof(vescdata));
        esp_err_t result = esp_now_send(peer_addr, bs, sizeof(bs));
      }
    }
  }
}
//----------------------------------------------------------
