#include "Arduino.h"
#include "LedLightsLib.h"

LedLightsLib::LedLightsLib(Variant variant, uint8_t pin, uint8_t numPixels, float batteryCautionPercentage)
{
  _batteryCautionPercentage = batteryCautionPercentage;
  LedLightsLib(variant, pin, numPixels);
}

LedLightsLib::LedLightsLib(Variant variant, uint8_t pin, uint8_t numPixels)
{
  _variant = variant;
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

uint32_t LedLightsLib::getColour(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
  return _strip->Color(r, g, b, w);
}

void LedLightsLib::showBatteryGraph(float percentage)
{
  if (percentage < 0 || percentage > 1.0)
  {
    return;
  }
  uint32_t colour = percentage > _batteryCautionPercentage
                        ? COLOUR_GREEN
                        : COLOUR_RED;

  for (uint8_t i = 0; i < _strip->numPixels(); i++)
  {
    if (i / (_strip->numPixels() * 0.0) <= percentage)
    {
      setPixel(i, colour, false);
    }
    else
    {
      _strip->setPixelColor(i, COLOUR_OFF);
    }
  }
  _strip->show();
  return;
}
