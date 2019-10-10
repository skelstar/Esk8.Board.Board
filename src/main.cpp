#include <Button2.h>
#include <Fsm.h>
#include <TaskScheduler.h>
#include <VescData.h>



#define ADC_EN 14
#define ADC_PIN 34
#define BUTTON_1 35
#define BUTTON_2 0

void button_init();
void button_loop();
void sleepThenWakeTimer(int ms);
void initialiseApp();


Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

#define NUM_PIXELS  21
#define PIXEL_PIN   25
#define BRIGHT_MAX  10

#include "vesc_utils.h"
#include "ble_notify.h"
#include "light-bar.h"

//------------------------------------------------------------------

enum EventsEnum
{
  INIT,
  POWER_UP,
  POWER_DOWN,
  VESC_OFFLINE,
  MOVING,
  STOPPED
} event;

State state_init([] {
    Serial.printf("State initialised\n");
  },
  NULL, NULL);

State state_power_up([] {
    Serial.printf("state_power_up\n");
  },
  NULL, NULL);

State state_power_down([] {
    Serial.printf("state_power_down\n");
  },
  NULL, NULL);

State state_vesc_online([] {
    Serial.printf("state_vesc_online\n");
  },
  NULL, NULL);

State state_vesc_offline([] {
    Serial.printf("state_vesc_offline\n");
  },
  NULL, NULL);



Fsm fsm(&state_init);

void addFsmTransitions()
{
  event = POWER_UP;
  fsm.add_transition(&state_init, &state_power_up, event, NULL);
  fsm.add_transition(&state_power_up, &state_power_up, event, NULL);

  event = POWER_DOWN;
  fsm.add_transition(&state_init, &state_power_down, event, NULL);
  fsm.add_transition(&state_power_up, &state_power_down, event, NULL);

  event = VESC_OFFLINE;
  fsm.add_transition(&state_init, &state_power_down, event, NULL);

  event = MOVING;
  fsm.add_transition(&state_init, &state_vesc_online, event, NULL);

  event = STOPPED;
  fsm.add_transition(&state_init, &state_vesc_online, event, NULL);
}
//------------------------------------------------------------------

Scheduler runner;

#define GET_FROM_VESC_INTERVAL 1000

Task t_GetVescValues(
    GET_FROM_VESC_INTERVAL,
    TASK_FOREVER,
    [] {
      bool vescOnline = getVescValues() == true;

      if (vescOnline == false)
      {
        Serial.printf("VESC not responding!\n");

        if (clientConnected) {
          vescdata.ampHours = dummyData.ampHours;
          vescdata.batteryVoltage = dummyData.batteryVoltage;
          vescdata.moving = dummyData.moving;
          vescdata.odometer = dummyData.odometer;
          sendDataToClient();
        }
        fsm.trigger(VESC_OFFLINE);
      }
      else
      {
        Serial.printf("batt volts: %.1f \n", vescdata.batteryVoltage);

        if (vescPoweringDown())
        {
          fsm.trigger(POWER_DOWN);
        }
        else if (vescdata.moving)
        {
          fsm.trigger(MOVING);
        }
        else
        {
          if (clientConnected) {
            sendDataToClient();
          }
          fsm.trigger(STOPPED);
        }
      }
    });

//------------------------------------------------------------------
//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void sleepThenWakeTimer(int ms)
{
  esp_sleep_enable_timer_wakeup(ms * 1000);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  esp_light_sleep_start();
}

void button_init()
{
  btn1.setPressedHandler([](Button2 &b) {
  });
  btn1.setReleasedHandler([](Button2 &b) {
  });
  // btn1.setClickHandler([](Button2 &b) {
  // });
  // btn1.setLongClickHandler([](Button2 &b) {
  //     // Serial.printf("btn1.setLongClickHandler([](Button2 &b)\n");
  // });
  // btn1.setDoubleClickHandler([](Button2 &b) {
  //     // Serial.printf("btn1.setDoubleClickHandler([](Button2 &b)\n");
  // });
  // btn1.setTripleClickHandler([](Button2 &b) {
  //     // Serial.printf("btn1.setTripleClickHandler([](Button2 &b)\n");
  // });

  btn2.setPressedHandler([](Button2 &b) {
  });
  btn2.setReleasedHandler([](Button2 &b) {
  });
}

void button_loop()
{
  btn1.loop();
  btn2.loop();
}

void initialiseApp()
{
  fsm.trigger(INIT);
  dummyData.ampHours = 2.1;
  dummyData.batteryVoltage = 32.3;
  dummyData.moving = false;
  dummyData.odometer = 1.23;
}

//----------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  Serial.println("Start");

  initialiseApp();

  FastLED.addLeds<WS2812B, PIXEL_PIN, GRB>(strip, NUM_PIXELS);
  FastLED.setBrightness(50);
  allLedsOn(COLOUR_WHITE);
	FastLED.show();

  setupBLE();

  vesc.init(VESC_UART_BAUDRATE);

  runner.startNow();
  runner.addTask(t_GetVescValues);
  t_GetVescValues.enable();

  addFsmTransitions();
  fsm.run_machine();

  button_init();
}
//----------------------------------------------------------
void loop()
{
  fsm.run_machine();

  runner.execute();

  button_loop();
}
//----------------------------------------------------------
