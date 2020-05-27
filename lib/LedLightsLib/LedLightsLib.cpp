#include "Arduino.h"
#include "LedLightsLib.h"

void LedLightsLib::initialise(uint8_t pin, uint8_t numPixels, uint8_t brightness)
{
  _strip = new Adafruit_NeoPixel(numPixels, pin, NEO_GRBW + NEO_KHZ800);
  _strip->begin();
  _brightness = brightness;
  _strip->setBrightness(_brightness);
  _strip->clear();
  _strip->show();
}

void LedLightsLib::setBrightness(uint8_t brightness)
{
  _brightness = brightness;
  _strip->setBrightness(_brightness);
}

void LedLightsLib::setAll(uint32_t colour)
{
  setAll(colour, 0, _strip->numPixels() - 1);
}

void LedLightsLib::setAll(uint32_t colour, uint8_t start, uint8_t end)
{
  if (_strip == NULL)
  {
    Serial.printf("ERROR: light not initialised");
    return;
  }

  if (start > end || end > _strip->numPixels() - 1)
  {
    Serial.printf("ERROR: start and/or end pixels are out of range!\n");
    return;
  }

  for (int i = start; i <= end; i++)
  {
    setPixel(i, colour, false);
  }
  _strip->show();
}

void LedLightsLib::setStatusIndicators(uint32_t vesc, uint32_t board, uint32_t controller)
{
  _strip->clear();

  int i = 3;
  setPixel(i++, controller, false);
  setPixel(i++, controller, false);
  setPixel(i++, controller, false);

  i = i + 2;
  setPixel(i++, board, false);
  setPixel(i++, board, false);
  setPixel(i++, board, false);

  i = i + 2;
  setPixel(i++, vesc, false);
  setPixel(i++, vesc, false);
  setPixel(i++, vesc, false);

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

void LedLightsLib::showBatteryGraph(float percentage, uint8_t start, uint8_t end)
{
  if (percentage < 0 || percentage > 1.0)
  {
    return;
  }
  uint8_t oldBrightness = _strip->getBrightness();
  _strip->setBrightness(100);

  // show bargraph in one ring
  uint8_t numRingPixels = end - start;

  uint8_t p = percentage * numRingPixels; // because two rings

  for (uint8_t i = 0; i < end - start; i++)
  {
    uint32_t c = (i <= p)
                     ? COLOUR_GREEN
                     : COLOUR_DARK_RED;
    _strip->setPixelColor(i + start, c);
  }
  _strip->show();
  // restore brightness
  _strip->setBrightness(oldBrightness);
  return;
}
