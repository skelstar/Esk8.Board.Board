
#ifndef CRGB
#include <FastLED.h>
#endif


CRGB strip[NUM_PIXELS];

uint32_t COLOUR_OFF = CRGB::Black;
uint32_t COLOUR_WHITE = CRGB::White;
uint32_t COLOUR_RED = CRGB::Red;


void allLedsOn(uint32_t colour) {
  for (int i=0; i<NUM_PIXELS; i++) {
    strip[i] = colour;
  }
  FastLED.show();
}
