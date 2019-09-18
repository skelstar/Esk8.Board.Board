#ifndef LedLightsLib_h
#define LedLightsLib_h

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class LedLightsLib 
{
    public:
		// enum StateCode {
		// 	ST_NOT_HELD,
		// 	EV_BUTTON_PRESSED,
		// 	ST_WAIT_FOR_RELEASE,
		// 	EV_SPECFIC_TIME_REACHED,
		// 	EV_RELEASED,
		// 	EV_HELD_SECONDS,
		// 	EV_DOUBLETAP
		// };

        uint32_t COLOUR_OFF = _strip->Color(0, 0, 0, 0);
        uint32_t COLOUR_RED = _strip->Color(0, 255, 0, 0);
        uint32_t COLOUR_GREEN = _strip->Color(0, 255, 0);
        uint32_t COLOUR_BLUE = _strip->Color(0, 0, 255, 0);
        uint32_t COLOUR_WHITE = _strip->Color(0, 0, 30, 255);


        LedLightsLib(uint8_t pin, uint8_t numPixels);
        void setAll(uint32_t colour);
        void setPixel(uint8_t pixel, uint32_t colour, bool show);
        uint32_t getColour(uint8_t r, uint8_t g, uint8_t b, uint8_t w);

    private:
        Adafruit_NeoPixel *_strip;
};

#endif