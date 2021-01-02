

#define LONGCLICK_MS 1000

#include <Button2.h>

#define BUTTON_0 0
#define PIN_04 04

#define PIN_37 37
#define PIN_38 38
#define PIN_39 39

Button2 button0(BUTTON_0);
Button2 primaryButton(PIN_04);

#ifdef USING_M5STACK
const uint8_t M5_BUTTON_A = PIN_39;
const uint8_t M5_BUTTON_B = PIN_38;
const uint8_t M5_BUTTON_C = PIN_37;

Button2 buttonA(M5_BUTTON_A);
Button2 buttonB(M5_BUTTON_B);
Button2 buttonC(M5_BUTTON_C);
#endif

/* prototypes */
void mockMoving(bool buttonHeld);

bool button0held = false;
//------------------------------------------------------------------

void button_init()
{
  button0.setPressedHandler([](Button2 &btn) {
  });

  button0.setReleasedHandler([](Button2 &btn) {
  });

  button0.setLongClickHandler([](Button2 &btn) {
  });
}
//------------------------------------------------------------------

#ifdef USING_M5STACK
void m5StackButtons_init()
{
  // ButtonA
  buttonA.setPressedHandler([](Button2 &btn) {
    if (MOCK_MOVING_WITH_BUTTON == 1)
      mockMoving(true);
  });
  buttonA.setReleasedHandler([](Button2 &btn) {
    if (MOCK_MOVING_WITH_BUTTON == 1)
      mockMoving(false);
  });
  buttonA.setLongClickHandler([](Button2 &btn) {
  });

  // ButtonB
  buttonB.setPressedHandler([](Button2 &btn) {
  });
  buttonB.setReleasedHandler([](Button2 &btn) {
    board_packet.command = CommandType::RESET;
    Serial.printf("sending command=RESET\n");
  });
  buttonB.setLongClickHandler([](Button2 &btn) {
  });

  // ButtonC
  buttonC.setPressedHandler([](Button2 &btn) {
  });
  buttonC.setReleasedHandler([](Button2 &btn) {
  });
  buttonC.setLongClickHandler([](Button2 &btn) {
  });
}
#endif
//------------------------------------------------------------------

void primaryButtonInit()
{
  primaryButton.setPressedHandler([](Button2 &btn) {
  });

  primaryButton.setReleasedHandler([](Button2 &btn) {
  });

  primaryButton.setLongClickHandler([](Button2 &btn) {
  });
}
//------------------------------------------------------------------

void mockMoving(bool buttonHeld)
{
  board_packet.moving = buttonHeld;
  if (buttonHeld)
  {
    if (board_packet.batteryVoltage <= 10.0)
      board_packet.batteryVoltage = 43.3;
    board_packet.motorCurrent = 3;
    board_packet.ampHours += board_packet.motorCurrent;
    // DEBUGVAL(
    //     board_packet.moving,
    //     board_packet.motorCurrent,
    //     board_packet.batteryVoltage,
    //     board_packet.ampHours);
    if (FEATURE_FOOTLIGHT)
      footlightQueue->send(FootLight::MOVING);

#if USING_M5STACK
    displayQueue->send(M5StackDisplay::Q_MOVING);
#endif
  }
  else
  {
    if (FEATURE_FOOTLIGHT)
      footlightQueue->send(FootLight::STOPPED);
#if USING_M5STACK
    displayQueue->send(M5StackDisplay::Q_STOPPED);
#endif
  }
}
//------------------------------------------------------
