
#ifndef Adafruit_NeoPixel
#include <Adafruit_NeoPixel.h>
#endif


Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIXEL_PIN, NEO_RGB + NEO_KHZ800);

uint32_t COLOUR_OFF = strip.Color(0, 0, 0);
uint32_t COLOUR_WHITE = strip.Color(BRIGHT_MAX-1, BRIGHT_MAX-1, BRIGHT_MAX-1);
uint32_t COLOUR_RED = strip.Color(BRIGHT_MAX-1, 0, 0);


void allLedsOn(uint32_t colour) {
  for (int i=0; i<NUM_PIXELS; i++) {
    strip.setPixelColor(i, colour);
  }
}
