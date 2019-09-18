#ifndef LedLightsLib_h
#define LedLightsLib_h

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class LedLightsLib 
{
    public:
        enum Variant {
            HEAD_LIGHT,
            TAIL_LIGHT
        };

        uint32_t COLOUR_OFF = _strip->Color(0, 0, 0, 0);
        uint32_t COLOUR_RED = _strip->Color(0, 255, 0, 0);
        uint32_t COLOUR_GREEN = _strip->Color(0, 255, 0);
        uint32_t COLOUR_BLUE = _strip->Color(0, 0, 255, 0);
        uint32_t COLOUR_WHITE = _strip->Color(0, 0, 30, 255);

        LedLightsLib(Variant variant, uint8_t pin, uint8_t numPixels, float batteryCautionPercentage);
        LedLightsLib(Variant variant, uint8_t pin, uint8_t numPixels);
        void setAll(uint32_t colour);
        void setPixel(uint8_t pixel, uint32_t colour, bool show);
        uint32_t getColour(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
        void showBatteryGraph(float percentage);

    private:
        Adafruit_NeoPixel *_strip;
        float _batteryCautionPercentage;
        Variant _variant;
};

#endif