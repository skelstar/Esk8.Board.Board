#include "Arduino.h"
#include "LedLightsLib.h"

LedLightsLib::LedLightsLib(uint8_t pin, uint8_t numPixels)
{
  _strip = new Adafruit_NeoPixel(numPixels, pin, NEO_GRBW + NEO_KHZ800);
  _strip->begin();
  _strip->clear();
  _strip->show();
}

void LedLightsLib::setAll(uint32_t colour)
{
  for (int i = 0; i < _strip->numPixels(); i++)
  {
    setPixel(i, colour, false);
  }
  _strip->show();
}

void LedLightsLib::setPixel(uint8_t pixel, uint32_t colour, bool show)
{
  _strip->setPixelColor(pixel, colour);
  if (show)
  {
    _strip->show();
  }
}

uint32_t LedLightsLib::getColour(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  return _strip->Color(r, g, b, w);
}
