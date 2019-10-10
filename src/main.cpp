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

boolean debugMode = false;
boolean debugPoweringDown = false;
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
  amphoursTotal = amphoursTotal > 0 ? amphoursTotal : 0.0;
  odometerTotal = odometerTotal > 0 ? odometerTotal : 0.0;
  storeFloat(STORE_AMP_HOURS_TOTAL, amphoursTotal + actualAmphours);
  storeFloat(STORE_ODOMETER_TOTAL, odometerTotal + actualOdometer);

  storeUInt8(STORE_POWERED_DOWN, 1); // true
  Serial.printf("Powering down. Storing:\n- ampHours = %.1fmAh (%.1fmAh)\n- odometer = %.1fkm (%.1fkm)\n",
                actualAmphours,
                initialVescData.ampHours,
                actualOdometer,
                initialVescData.odometer);
  Serial.printf("Total stats:\n- %.1fmAh (%.1fmAh + %.1fmAh)\n- %.1fkm (%.1fkm + %.1fkm)\n",
    amphoursTotal + actualAmphours, 
    amphoursTotal,
    actualAmphours, 
    odometerTotal + actualOdometer,
    odometerTotal,
    actualOdometer);
}

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
  saveTripToMemory();
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
      bool vescOnline = getVescValues() == true && !btn1.isPressed();

      if (debugMode && !btn1.isPressed())
      {
        vescOnline = true;
        if (!debugPoweringDown)
        {
          vescdata.ampHours = vescdata.ampHours > 0.0 ? vescdata.ampHours + 0.23 : 12.0;
          vescdata.odometer = vescdata.odometer > 0.0 ? vescdata.odometer + 0.1 : 1.0;
          vescdata.batteryVoltage = vescdata.batteryVoltage > 30 && vescdata.batteryVoltage < 46 && !debugPoweringDown
                                        ? vescdata.batteryVoltage + 0.1
                                        : POWERING_DOWN_BATT_VOLTS_START + 1.0;
          Serial.printf("DEBUG: ampHours %.1fmAh odo %.1fkm batt: %.1fv \n", vescdata.ampHours, vescdata.odometer, vescdata.batteryVoltage);
        }
        else
        {
          vescdata.batteryVoltage = vescdata.batteryVoltage > 0 
            ? vescdata.batteryVoltage - 5 
            : 0;
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
    debugMode = !debugMode;
    Serial.printf("DEBUG: %d\n", debugMode);
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
    debugPoweringDown = !debugPoweringDown;
    if (debugPoweringDown)
    {
      Serial.printf("debugPoweringDown!\n");
    }
    else
    {
      Serial.printf("batt restored\n");
    }
  });
  // btn2.setReleasedHandler([](Button2 &b) {
  // });
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

  button_loop();
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