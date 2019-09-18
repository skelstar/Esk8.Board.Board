#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "LedLightsLib.h"

/*--------------------------------------------------------------------------------*/

#define variant light.HEAD_LIGHT
LedLightsLib light(variant, /*pin*/ 32, /*num*/ 15, /*batt caution%*/ 0.3);

/*--------------------------------------------------------------------------------*/

const char compile_date[] = __DATE__ " " __TIME__;
const char file_name[] = __FILE__;

//--------------------------------------------------------------

void setup()
{
	Serial.begin(115200);
  Serial.println("\nStarting Esk8.Board.Server!");

  light.setAll(light.COLOUR_WHITE);
  delay(2000);
  light.showBatteryGraph(0.8);
}

void loop() {

  delay(10);

}
