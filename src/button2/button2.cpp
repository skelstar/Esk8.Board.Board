#include <Arduino.h>
#include <SPI.h>
#include <Button2.h>
#include <elapsedMillis.h>

#define BUTTON_PRIMARY 21
const uint8_t M5_BUTTON_A = 39;
const uint8_t M5_BUTTON_B = 38;
const uint8_t M5_BUTTON_C = 37;

Button2 button(M5_BUTTON_A); //, INPUT_PULLUP, false, /*acitve low*/ true);

void setup()
{
  Serial.begin(115200);
  Serial.printf("Ready\n");

  button.setClickHandler([](Button2 &btn) {
    Serial.printf("click\n");
  });
  button.setPressedHandler([](Button2 &btn) {
    Serial.printf("Pressed\n");
  });
  button.setReleasedHandler([](Button2 &btn) {
    Serial.printf("Release\n");
  });

  // button.setActiveLow(true);
}

elapsedMillis sinceCheckedPressed;

void loop()
{
  button.loop();
  if (sinceCheckedPressed > 1000)
  {
    sinceCheckedPressed = 0;
    Serial.printf("  isPressed(): %s raw(): %s pin: %d\n",
                  button.isPressed() ? "TRUE" : "FALSE",
                  button.isPressedRaw() ? "TRUE" : "FALSE",
                  digitalRead(BUTTON_PRIMARY));
  }
}