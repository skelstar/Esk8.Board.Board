
#define LONGCLICK_MS 1000

void button_init()
{
  button0.setPressedHandler([](Button2 &btn) {
    EventsEnum e = EV_MOVING;
    vescdata.moving = true;
    NRF_EVENT(EV_NRF_SEND_MOVING, "EV_NRF_SEND_MOVING");
  });
  button0.setReleasedHandler([](Button2 &btn) {
    EventsEnum e = EV_STOPPED;
    vescdata.odometer = vescdata.odometer + 0.1;
    vescdata.moving = false;
    NRF_EVENT(EV_NRF_SEND_STOPPED, "EV_NRF_SEND_STOPPED");
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
