#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

/*--------------------------------------------------------------------------------*/

#define NUM_PIXELS  15
#define PIXEL_PIN  32

#define BRIGHTNESS  100

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIXEL_PIN, NEO_GRBW + NEO_KHZ800);

uint32_t COLOUR_OFF = strip.Color(0, 0, 0, 0);
uint32_t COLOUR_RED = strip.Color(0, 255, 0, 0);
uint32_t COLOUR_GREEN = strip.Color(0, 255, 0);
uint32_t COLOUR_BLUE = strip.Color(0, 0, 255, 0);
uint32_t COLOUR_WHITE = strip.Color(0, 0, 30, 255);


void setPixel(int i, uint32_t c, bool show);
void setAllPixels(uint32_t c);

/*--------------------------------------------------------------------------------*/

const char compile_date[] = __DATE__ " " __TIME__;
const char file_name[] = __FILE__;

//--------------------------------------------------------------

void setup()
{
	Serial.begin(115200);
  Serial.println("\nStarting Esk8.Board.Server!");

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  setAllPixels(COLOUR_WHITE);
}

void loop() {

  delay(10);
}

void setPixel(int i, uint32_t c, bool show) {
  strip.setPixelColor(i, c);
  if (show) {
    strip.show();
  }
}

void setAllPixels(uint32_t c) {
  for (int i=0; i<NUM_PIXELS; i++) {
    setPixel(i, c, false);
  }
  strip.show();
}