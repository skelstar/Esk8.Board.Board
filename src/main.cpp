#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "LedLightsLib.h"
#include <Fsm.h>

/*--------------------------------------------------------------------------------*/

#define variant light.HEAD_LIGHT
LedLightsLib light(variant, /*pin*/ 32, /*num*/ 15, /*batt caution%*/ 0.3);

#define BATTERY_MAX   42.6
#define BATTERY_WARN  37.4
#define BATTERY_MIN   34.0

/*--------------------------------------------------------------------------------*/

const char compile_date[] = __DATE__ " " __TIME__;
const char file_name[] = __FILE__;

//--------------------------------------------------------------

void headlightsOn() {
  light.setAll(light.COLOUR_WHITE);
}

void showBatteryStatus() {
  float batteryVolts = 38.9;
  float battPercent = (batteryVolts-BATTERY_MIN) / (BATTERY_MAX-BATTERY_MIN);
  light.showBatteryGraph(battPercent);
}

//--------------------------------------------------------------

enum EventsEnum
{
  POWER_UP,
  BOARD_UP,
  BOARD_DOWN,
  POWER_DOWN
} event;

State state_init([] {
    Serial.printf("State initialised");
  },
  NULL,
  NULL
);

State state_board_up([] {
    Serial.printf("Board tipped up\n");
    showBatteryStatus();
  }, 
  NULL, 
  NULL
);

State state_board_down([] {
    Serial.printf("Board lowered down\n");
    headlightsOn();
  }, 
  NULL, 
  NULL
);

Fsm fsm(&state_init);

void addFsmTransitions() {
  uint8_t event = POWER_UP;
  fsm.add_transition(&state_init, &state_board_down, event, NULL);

  event = BOARD_DOWN;
  fsm.add_transition(&state_init, &state_board_down, event, NULL);
  fsm.add_transition(&state_board_up, &state_board_down, event, NULL);

  event = BOARD_UP;
  fsm.add_transition(&state_init, &state_board_up, event, NULL);
  fsm.add_transition(&state_board_down, &state_board_up, event, NULL);

  event = POWER_DOWN;
}

void setup()
{
	Serial.begin(115200);
  Serial.println("\nStarting Esk8.Board.Server!");

  addFsmTransitions();
  fsm.run_machine();
}

void loop() {

  // serviceIMU()
  fsm.run_machine();
  delay(10);
}
