#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>

//------------------------
#define PIXEL_PIN 26
#define NUM_PIXELS 12 + 10 + 12 // 12 per ring, 10 in the centre
//------------------------

#include <LedLightsLib.h>
LedLightsLib lights;

#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel strip(NUM_PIXELS, PIXEL_PIN, NEO_GRBW + NEO_KHZ800);

elapsedMillis since_refreshed_light;

void setup()
{
  Serial.begin(115200);
  DEBUG("Ready");

  lights.initialise(PIXEL_PIN, NUM_PIXELS * 2, /*brightness*/ 30);
  // lights.setAll(lights.COLOUR_COLD_WHITE);

  since_refreshed_light = 3000;
}

void loop()
{
  if (since_refreshed_light > 3000)
  {
    since_refreshed_light = 0;
    lights.setAll(lights.COLOUR_WHITE, 0, 12 - 1);
    lights.setAll(lights.COLOUR_RED, 12, 12 + 10 - 1);
    lights.setAll(lights.COLOUR_BLUE, 12 + 10, 12 + 10 + 12);
  }
  vTaskDelay(10);
}