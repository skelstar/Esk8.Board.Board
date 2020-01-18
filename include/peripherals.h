
#define LONGCLICK_MS 1000

#include <Button2.h>

#define BUTTON_0  0

Button2 button0(BUTTON_0);

void button_init()
{
  button0.setPressedHandler([](Button2 &btn) {
    board_packet.moving = true;
    send_packet_to_controller(ReasonType::BOARD_MOVING);
  });
  button0.setReleasedHandler([](Button2 &btn) {
    board_packet.odometer = board_packet.odometer + 0.1;
    board_packet.moving = false;
    send_packet_to_controller(ReasonType::BOARD_STOPPED);
  });
  button0.setLongClickHandler([](Button2 &btn) {
  });
}

//------------------------------------------------------------------
void light_init()
{
  // light.initialise(PIXEL_PIN, NUM_PIXELS);
  // light.setAll(light.COLOUR_OFF);
}
