#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>

//------------------------
#define PIXEL_PIN 26
#define NUM_PIXELS 13 // per ring
//------------------------

#include <LedLightsLib.h>
LedLightsLib lights;

void setup()
{
  Serial.begin(115200);
  DEBUG("Ready");

  lights.initialise(PIXEL_PIN, NUM_PIXELS * 2, /*brightness*/ 100);
  lights.setAll(lights.COLOUR_COLD_WHITE);
}

elapsedMillis since_sent_to_board;

void loop()
{
  vTaskDelay(10);
}