#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>
#include <Button2.h>
#include "esp_adc_cal.h"
#include "bmp.h"
#include <Fsm.h>
#include "display_utils.h"

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

Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

void button_init();
void button_loop();
void sleepThenWakeTimer(int ms);
void initDisplay();
void displayPopup(char* message);
void drawBattery(int percent);

//------------------------------------------------------------------
enum EventsEnum
{
  POWER_UP,
  BOARD_UP,
  BOARD_DOWN,
  POWER_DOWN
} event;

State state_init([] {
    Serial.printf("State initialised");
  },
  NULL,
  NULL
);

State state_board_up([] {
  }, 
  NULL, 
  NULL
);

State state_board_down([] {
  }, 
  NULL, 
  NULL
);

Fsm fsm(&state_init);

void addFsmTransitions() {
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
    btn1.setClickHandler([](Button2 &b){
        Serial.printf("btn1.setClickHandler([](Button2 &b)\n");
    });
    btn1.setLongClickHandler([](Button2 &b) {
        Serial.printf("btn1.setLongClickHandler([](Button2 &b)\n");
    });
    btn1.setDoubleClickHandler([](Button2 &b){
        Serial.printf("btn1.setDoubleClickHandler([](Button2 &b)\n");
    });
    btn1.setTripleClickHandler([](Button2 &b){
        Serial.printf("btn1.setTripleClickHandler([](Button2 &b)\n");
    });
    btn1.setReleasedHandler([](Button2 &b){
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

    addFsmTransitions();
    fsm.run_machine();

    initDisplay();

    button_init();

    displayPopup("Ready");

    drawBattery(65);
}

void loop()
{
    fsm.run_machine();

    button_loop();
}
//----------------------------------------------------------
void initDisplay() {
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

void displayPopup(char* message) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(4);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(message, TFT_HEIGHT/2, TFT_WIDTH/2);
}

//--------------------------------------------------------------------------------

#define SCREEN_WIDTH    TFT_HEIGHT
#define SCREEN_HEIGHT   TFT_WIDTH
#define BATTERY_WIDTH 200
#define BATTERY_HEIGHT 90
#define BORDER_SIZE 10
#define KNOB_HEIGHT 30

void drawBattery(int percent)
{
    int outsideX = (SCREEN_WIDTH - (BATTERY_WIDTH + BORDER_SIZE)) / 2;
    int outsideY = (SCREEN_HEIGHT - BATTERY_HEIGHT) / 2;
    Serial.printf("\n BATTERY_WIDTH %d BATTERY_HEIGHT %d outsideX %d outsideY %d \n", BATTERY_WIDTH, BATTERY_HEIGHT, outsideX, outsideY);
    // clear
    tft.fillScreen(TFT_BLACK);
    // body
    tft.fillRect(outsideX, outsideY, BATTERY_WIDTH, BATTERY_HEIGHT, TFT_WHITE);
    // knob
    tft.fillRect(
        outsideX + BATTERY_WIDTH,
        outsideY + (BATTERY_HEIGHT - KNOB_HEIGHT) / 2,
        BORDER_SIZE,
        KNOB_HEIGHT,
        TFT_WHITE);
    // body-inside
    tft.fillRect(
        outsideX + BORDER_SIZE,
        outsideY + BORDER_SIZE,
        BATTERY_WIDTH - BORDER_SIZE * 2,
        BATTERY_HEIGHT - BORDER_SIZE * 2,
        TFT_BLACK);
    // capacity
    tft.fillRect(
        outsideX + BORDER_SIZE * 2,
        outsideY + BORDER_SIZE * 2,
        (BATTERY_WIDTH - BORDER_SIZE * 4) * percent / 100,
        BATTERY_HEIGHT - BORDER_SIZE * 4,
        TFT_WHITE);
}
//--------------------------------------------------------------------------------