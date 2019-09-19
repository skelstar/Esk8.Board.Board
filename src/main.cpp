#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "LedLightsLib.h"
#include <Fsm.h>

/*--------------------------------------------------------------------------------*/

#define variant light.HEAD_LIGHT
LedLightsLib light(variant, /*pin*/ 32, /*num*/ 15, /*batt caution%*/ 0.3);

/*--------------------------------------------------------------------------------*/

const char compile_date[] = __DATE__ " " __TIME__;
const char file_name[] = __FILE__;

//--------------------------------------------------------------



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
  // change lights - show battery charge
    Serial.printf("Board tipped up\n");
  }, 
  NULL, 
  NULL
);

State state_board_down([] {
    // head/tail lights on
    Serial.printf("Board lowered down\n");
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

  light.setAll(light.COLOUR_WHITE);
  delay(2000);
  light.showBatteryGraph(0.8);
}

void loop() {

  // serviceIMU()
  fsm.run_machine();
  delay(10);
}
