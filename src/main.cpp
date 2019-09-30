#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "LedLightsLib.h"
#include <Fsm.h>

#include <SPI.h>
#include <epd2in13.h>
#include <epdpaint.h>

/*--------------------------------------------------------------------------------*/

#define variant light.HEAD_LIGHT
LedLightsLib light(variant, /*pin*/ 32, /*num*/ 15, /*batt caution%*/ 0.3);

#define BATTERY_MAX   42.6
#define BATTERY_WARN  37.4
#define BATTERY_MIN   34.0

/*--------------------------------------------------------------------------------*/

#define COLORED     0
#define UNCOLORED   1

unsigned char image[1024];
Paint paint(image, 0, 0);
Epd epd;
unsigned long time_start_ms;
unsigned long time_now_s;
/*--------------------------------------------------------------------------------*/

const char compile_date[] = __DATE__ " " __TIME__;
const char file_name[] = __FILE__;

//--------------------------------------------------------------

void headlightsOn() {
  light.setAll(light.COLOUR_WHITE);
}

void showBatteryStatus() {
  float batteryVolts = 38.9;
  float battPercent = (batteryVolts-BATTERY_MIN) / (BATTERY_MAX-BATTERY_MIN);
  light.showBatteryGraph(battPercent);
}

//--------------------------------------------------------------

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
    Serial.printf("Board tipped up\n");
    showBatteryStatus();
  }, 
  NULL, 
  NULL
);

State state_board_down([] {
    Serial.printf("Board lowered down\n");
    headlightsOn();
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

void setupEpd();

void setup()
{
	Serial.begin(115200);
  Serial.println("\nStarting Esk8.Board.Server!");

  setupEpd();
  Serial.printf("Initialised epd\n");

  addFsmTransitions();
  fsm.run_machine();
}

void loop() {

  // serviceIMU()
  fsm.run_machine();
  delay(10);
}


void setupEpd() {
  if (epd.Init(lut_full_update) != 0) {
      Serial.print("e-Paper init failed");
      return;
  }

  epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
  
  paint.SetRotate(ROTATE_0);
  paint.SetWidth(128);    // width should be the multiple of 8 
  paint.SetHeight(24);

  /* For simplicity, the arguments are explicit numerical coordinates */
  paint.Clear(COLORED);
  paint.DrawStringAt(30, 4, "Hello world!", &Font12, UNCOLORED);
  epd.SetFrameMemory(paint.GetImage(), 0, 10, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(30, 4, "e-Paper Demo", &Font12, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 0, 30, paint.GetWidth(), paint.GetHeight());

  paint.SetWidth(64);
  paint.SetHeight(64);
  
  paint.Clear(UNCOLORED);
  paint.DrawRectangle(0, 0, 40, 50, COLORED);
  paint.DrawLine(0, 0, 40, 50, COLORED);
  paint.DrawLine(40, 0, 0, 50, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 16, 60, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawCircle(32, 32, 30, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 72, 60, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawFilledRectangle(0, 0, 40, 50, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 16, 130, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawFilledCircle(32, 32, 30, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 72, 130, paint.GetWidth(), paint.GetHeight());
  epd.DisplayFrame();

  delay(500);
  epd.DisplayFrame();

  Serial.printf("here\n");

  // delay(2000);

  // if (epd.Init(lut_partial_update) != 0) {
  //     Serial.print("e-Paper init failed");
  //     return;
  // }

  /** 
   *  there are 2 memory areas embedded in the e-paper display
   *  and once the display is refreshed, the memory area will be auto-toggled,
   *  i.e. the next action of SetFrameMemory will set the other memory area
   *  therefore you have to set the frame memory and refresh the display twice.
   */
  // epd.SetFrameMemory(IMAGE_DATA);
  // epd.DisplayFrame();
  // epd.SetFrameMemory(IMAGE_DATA);
  // epd.DisplayFrame();
}