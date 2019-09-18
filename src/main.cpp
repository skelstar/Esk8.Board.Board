#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "LedLightsLib.h"

/*--------------------------------------------------------------------------------*/

LedLightsLib strip(/*pin*/ 32, /*num*/ 15);

/*--------------------------------------------------------------------------------*/

const char compile_date[] = __DATE__ " " __TIME__;
const char file_name[] = __FILE__;

//--------------------------------------------------------------

void setup()
{
	Serial.begin(115200);
  Serial.println("\nStarting Esk8.Board.Server!");

  strip.setAll(strip.COLOUR_WHITE);
}

void loop() {

  delay(10);
}
