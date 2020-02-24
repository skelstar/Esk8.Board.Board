#ifndef LedLightsLib_h
#define LedLightsLib_h

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class LedLightsLib
{
public:
    uint32_t COLOUR_OFF = _strip->Color(0, 0, 0, 0);
    uint32_t COLOUR_RED = _strip->Color(50, 0, 0, 0);
    uint32_t COLOUR_GREEN = _strip->Color(0, 50, 0, 0);
    uint32_t COLOUR_BLUE = _strip->Color(0, 0, 50, 0);
    uint32_t COLOUR_WHITE = _strip->Color(0, 0, 0, 50);

    void initialise(uint8_t pin, uint8_t numPixels);
    void setStatusIndicators(uint32_t vesc, uint32_t board, uint32_t controller);
    void setAll(uint32_t colour);
    void setPixel(uint8_t pixel, uint32_t colour, bool show);
    uint32_t getColour(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
    void showBatteryGraph(float percentage);

private:
    Adafruit_NeoPixel *_strip;
    float _batteryCautionPercentage;
};

#endif