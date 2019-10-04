#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>
#include <Button2.h>
#include "esp_adc_cal.h"
#include "bmp.h"
#include <Fsm.h>
#include <TaskScheduler.h>

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN 0x10
#endif

#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 16
#define TFT_RST 23

#define TFT_BL 4 // Display backlight control pin
#define ADC_EN 14
#define ADC_PIN 34
#define BUTTON_1 35
#define BUTTON_2 0

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

#include "display_utils.h"
#include "vesc_utils.h"

Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

void button_init();
void button_loop();
void sleepThenWakeTimer(int ms);
void initDisplay();
void displayPopup(char *message);
void drawBattery(int percent);

//------------------------------------------------------------------

Scheduler runner;

Task t_GetVescValues(
    GET_FROM_VESC_INTERVAL, 
    TASK_FOREVER, 
    [] {
        bool vescOnline = getVescValues() == true;

        if (vescOnline == false)
        {
            // Serial.printf("VESC not responding!\n");
        }
        else
        {
            Serial.printf("batt volts: %.1f \n", vescdata.batteryVoltage);

            if (vescPoweringDown())
            {
            }
            else if (vescdata.moving == false)
            {
            }
            else
            {
            }
        }
    });

//------------------------------------------------------------------

Fsm vescFsm(&state_board_offline);

enum EventsEnum
{
    POWER_UP,
    BOARD_UP,
    BOARD_DOWN,
    POWER_DOWN
} event;

State state_init([] {
    // Serial.printf("State initialised");
},
NULL, NULL);

State state_board_up([] {
},
NULL, NULL);

State state_board_down([] {
},
NULL, NULL);

Fsm fsm(&state_init);

void addFsmTransitions()
{
    uint8_t event = POWER_UP;
    fsm.add_transition(&state_init, &state_board_down, event, NULL);

    event = BOARD_DOWN;
    fsm.add_transition(&state_init, &state_board_down, event, NULL);
    fsm.add_transition(&state_board_up, &state_board_down, event, NULL);

    event = BOARD_UP;
    fsm.add_transition(&state_init, &state_board_up, event, NULL);
    fsm.add_transition(&state_board_down, &state_board_up, event, NULL);

    event = POWER_DOWN;
}
//------------------------------------------------------------------
//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void sleepThenWakeTimer(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

void button_init()
{
    btn1.setPressedHandler([] (Button2 &b) {
        vescFsm.trigger(ONLINE);
    });
    btn1.setReleasedHandler([](Button2 &b) {
        vescFsm.trigger(OFFLINE);
    });
    // btn1.setClickHandler([](Button2 &b) {
    // });
    // btn1.setLongClickHandler([](Button2 &b) {
    //     // Serial.printf("btn1.setLongClickHandler([](Button2 &b)\n");
    // });
    // btn1.setDoubleClickHandler([](Button2 &b) {
    //     // Serial.printf("btn1.setDoubleClickHandler([](Button2 &b)\n");
    // });
    // btn1.setTripleClickHandler([](Button2 &b) {
    //     // Serial.printf("btn1.setTripleClickHandler([](Button2 &b)\n");
    // });

    btn2.setPressedHandler([](Button2 &b) {
        vescFsm.trigger(MOVING);
    });
    btn2.setReleasedHandler([](Button2 &b) {
        vescFsm.trigger(STOPPED);
    });
}

void button_loop()
{
    btn1.loop();
    btn2.loop();
}
//----------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    Serial.println("Start");

    vesc.init(VESC_UART_BAUDRATE);

    runner.startNow();
    runner.addTask(t_GetVescValues);
    t_GetVescValues.enable();

    addFsmTransitions();
    fsm.run_machine();

    addVescFsmTransitions(&vescFsm);
    vescFsm.run_machine();

    initDisplay();

    vescFsm.trigger(OFFLINE);

    button_init();

    waitForFirstPacketFromVesc();

}

void loop()
{
    fsm.run_machine();
    vescFsm.run_machine();

    runner.execute();

    button_loop();
}
//----------------------------------------------------------
void initDisplay()
{
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(0, 0);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);

    if (TFT_BL > 0)
    {                                           // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
        pinMode(TFT_BL, OUTPUT);                // Set backlight pin to output mode
        digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
    }

    tft.setSwapBytes(true);
    tft.pushImage(0, 0, 240, 135, ttgo);
    sleepThenWakeTimer(1000);
}

//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------