
#define LONGCLICK_MS 1000

#include <Button2.h>

#define BUTTON_0 0
#define PIXEL_PIN 27            //4
#define NUM_PIXELS 12 + 10 + 12 // 12 per ring, 10 in the centre

Button2 button0(BUTTON_0);

bool button0held = false;

void button_init()
{
  button0.setPressedHandler([](Button2 &btn) {
#ifdef BUTTON_MOVING
    board_packet.moving = true;
    board_packet.motorCurrent += 0.1;
#endif
  });
  button0.setReleasedHandler([](Button2 &btn) {
#ifdef BUTTON_MOVING
    board_packet.odometer = board_packet.odometer + 0.1;
    board_packet.moving = false;
#endif
  });
  button0.setLongClickHandler([](Button2 &btn) {
  });
}

//------------------------------------------------------------------
void light_init()
{
  light.initialise(PIXEL_PIN, NUM_PIXELS * 2, /*brightness*/ 100);
  light.setAll(light.COLOUR_OFF);
}
