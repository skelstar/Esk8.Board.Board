#include <Button2.h>
#include <Fsm.h>
#include <TaskScheduler.h>
#include <VescData.h>

#define ADC_EN 14
#define ADC_PIN 34
#define BUTTON_1 35
#define BUTTON_2 0

volatile bool responded = true;

void button_init();
void button_loop();
void sleepThenWakeTimer(int ms);
void initialiseApp();

// #define USING_BUTTONS true
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

#define NUM_PIXELS  21
#define PIXEL_PIN   5
#define BRIGHT_MAX  10


boolean debugMode = false;
boolean debugPoweringDown = false;
boolean vescConnected = false;
float lastStableVolts = 0.0;

#include "vesc_utils.h"
#include "utils.h"
#include "light-bar.h"

#include "ble_notify.h"

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
  if (vescConnected)
  {
    lastStableVolts = vescdata.batteryVoltage;
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

Scheduler runner;

#define GET_FROM_VESC_INTERVAL 1000

Task t_GetVescValues(
    GET_FROM_VESC_INTERVAL,
    TASK_FOREVER,
    [] {
      // btn1 can put vesc into offline
      bool vescOnline = getVescValues() == true;  // && !btn1.isPressed();

      if (debugMode)
      {
        vescOnline = true;
        fakeVescData();
        if (debugPoweringDown && vescdata.batteryVoltage == 0.0) {
          btStop();
        }
      }

      if (vescOnline == false)
      {
        vescConnected = false;
        fsm.trigger(VESC_OFFLINE);
      }
      else
      {
        bool firstPacket = vescConnected == false && initialVescData.ampHours <= 0.0;

        if (firstPacket)
        {
          initialVescData.ampHours = vescdata.ampHours;
          initialVescData.odometer = vescdata.odometer;
        }
        vescConnected = true;

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

      if (bleClientConnected)
      {
        sendDataToClient();
      }

      fsm.run_machine();
    });

//------------------------------------------------------------------
//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void sleepThenWakeTimer(int ms)
{
  esp_sleep_enable_timer_wakeup(ms * 1000);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  esp_light_sleep_start();
}

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
    Serial.printf("btn1.setClickHandler()\n");
  });
  btn1.setLongClickHandler([](Button2 &b) {
    if (!debugMode) {
      debugMode = true;
      Serial.printf("DEBUG: %d\n", debugMode);
    }
    else if (debugMode && !debugPoweringDown) {
      Serial.printf("DEBUG: powering down\n");
      debugPoweringDown = true;
    }
    Serial.printf("btn1.setLongClickHandler([](Button2 &b)\n");
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
  btn2.loop();
}

void initialiseApp()
{
  fsm.trigger(WAITING_FOR_VESC);
}

void initialiseLeds() {
  FastLED.addLeds<WS2812B, PIXEL_PIN, GRB>(strip, NUM_PIXELS);
  FastLED.setBrightness(50);
  allLedsOn(COLOUR_RED);
  FastLED.show();
}

//----------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  Serial.println("Start");

  initialiseApp();

  initialiseLeds();

  setupBLE();

  vesc.init(VESC_UART_BAUDRATE);

  runner.startNow();
  runner.addTask(t_GetVescValues);
  t_GetVescValues.enable();

  addFsmTransitions();
  fsm.run_machine();

  #ifdef USING_BUTTONS
  button_init();
  button_loop();
  #endif
  //waitForFirstPacketFromVesc();
}
//----------------------------------------------------------
void loop()
{
  fsm.run_machine();

  runner.execute();

  button_loop();
}
//----------------------------------------------------------
