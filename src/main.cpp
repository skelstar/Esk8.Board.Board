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

#define STORE_NAMESPACE "data"
#define STORE_AMP_HOURS_TRIP "trip-amphours"
#define STORE_AMP_HOURS_TOTAL "total-amphours"
#define STORE_ODOMETER_TRIP "trip-odometer"
#define STORE_ODOMETER_TOTAL "total-odometer"

#define STORE_POWERED_DOWN "poweredDown"
#define STORE_LAST_VOLTAGE_READ "lastVolts"

Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

boolean vescConnected = false;
float lastStableVolts = 0.0;

#include "vesc_utils.h"
#include "ble_notify.h"
#include "nvmstorage.h"

void saveTripToMemory()
{
  // trip
  float actualAmphours = vescdata.ampHours - initialVescData.ampHours;
  float actualOdometer = vescdata.odometer - initialVescData.odometer;
  storeFloat(STORE_AMP_HOURS_TRIP, actualAmphours);
  storeFloat(STORE_ODOMETER_TRIP, actualOdometer);
  storeFloat(STORE_LAST_VOLTAGE_READ, lastStableVolts);
  // total
  float amphoursTotal = recallFloat(STORE_AMP_HOURS_TOTAL);
  float odometerTotal = recallFloat(STORE_ODOMETER_TOTAL);
  storeFloat(STORE_AMP_HOURS_TOTAL, amphoursTotal > 0 ? amphoursTotal : 0.0);
  storeFloat(STORE_ODOMETER_TOTAL, odometerTotal > 0 ? odometerTotal : 0.0);

  storeUInt8(STORE_POWERED_DOWN, 1); // true
  Serial.printf("Powering down. Storing totalAmpHours (%.1f + %.1f)\n", actualAmphours, vescdata.ampHours);
}

//------------------------------------------------------------------

enum EventsEnum
{
  INIT,
  POWER_UP,
  POWERING_DOWN,
  VESC_OFFLINE,
  MOVING,
  STOPPED
} event;

State state_init([] {
  Serial.printf("state_init\n");
},
NULL, NULL);

State state_powering_up([] {
  Serial.printf("state_powering_up\n");
},
NULL, NULL);

State state_powering_down([] {
  Serial.printf("state_powering_down\n");
  saveTripToMemory();
},
NULL, NULL);

State state_offline([] {
  Serial.printf("state_offline\n");
},
NULL, NULL);

State state_board_moving([] {
  Serial.printf("state_board_moving\n");
},
NULL, NULL);

State state_board_stopped([] {
  Serial.printf("state_board_stopped\n");
  if (vescConnected)
  {
    lastStableVolts = vescdata.batteryVoltage;
  }
},
NULL, NULL);

State state_vesc_offline(
  [] { Serial.printf("state_vesc_offline\n"); },
  [] {
    if (clientConnected)
    {
      // vescdata.ampHours = dummyData.ampHours;
      // vescdata.batteryVoltage = dummyData.batteryVoltage;
      // vescdata.moving = dummyData.moving;
      // vescdata.odometer = dummyData.odometer;
      // sendDataToClient();
    }
  },
  NULL);

Fsm fsm(&state_init);

void addFsmTransitions()
{
  event = POWER_UP;
  fsm.add_transition(&state_init, &state_powering_up, event, NULL);
  fsm.add_transition(&state_powering_up, &state_powering_up, event, NULL);

  event = POWERING_DOWN;
  fsm.add_transition(&state_init, &state_powering_down, event, NULL);
  fsm.add_transition(&state_powering_up, &state_powering_down, event, NULL);
  fsm.add_transition(&state_board_moving, &state_powering_down, event, NULL);
  fsm.add_transition(&state_board_stopped, &state_powering_down, event, NULL);

  event = VESC_OFFLINE;
  fsm.add_transition(&state_init, &state_powering_down, event, NULL);
  fsm.add_transition(&state_board_moving, &state_offline, event, NULL);
  fsm.add_transition(&state_board_stopped, &state_offline, event, NULL);

  event = MOVING;
  fsm.add_transition(&state_init, &state_board_moving, event, NULL);
  fsm.add_transition(&state_board_stopped, &state_board_moving, event, NULL);

  event = STOPPED;
  fsm.add_transition(&state_init, &state_board_stopped, event, NULL);
  fsm.add_transition(&state_board_moving, &state_board_stopped, event, NULL);
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
        vescConnected = false;
        fsm.trigger(VESC_OFFLINE);
      }
      else
      {
        Serial.printf("batt volts: %.1f \n", vescdata.batteryVoltage);

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
          if (clientConnected)
          {
            sendDataToClient();
          }
        }
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

  setupBLE();

  vesc.init(VESC_UART_BAUDRATE);

  runner.startNow();
  runner.addTask(t_GetVescValues);
  t_GetVescValues.enable();

  addFsmTransitions();
  fsm.run_machine();

  button_init();

  //waitForFirstPacketFromVesc();
}

void loop()
{
  fsm.run_machine();

  runner.execute();

  button_loop();
}
//----------------------------------------------------------

//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------