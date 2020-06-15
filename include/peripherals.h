
#define LONGCLICK_MS 1000

#include <Button2.h>

#define BUTTON_0 0

Button2 button0(BUTTON_0);

bool button0held = false;

void button_init()
{
  button0.setPressedHandler([](Button2 &btn) {
#ifdef BUTTON_MOVING
    board_packet.moving = true;
    board_packet.motorCurrent += 0.1;
    DEBUGVAL(board_packet.moving, board_packet.motorCurrent);
    sendToFootLightEventQueue(FootLightEvent::EV_MOVING);
#endif
  });
  button0.setReleasedHandler([](Button2 &btn) {
#ifdef BUTTON_MOVING
    board_packet.odometer = board_packet.odometer + 0.1;
    board_packet.moving = false;
    DEBUGVAL(board_packet.moving, board_packet.motorCurrent);
    sendToFootLightEventQueue(FootLightEvent::EV_STOPPED);
#endif
  });
  button0.setLongClickHandler([](Button2 &btn) {
  });
}
