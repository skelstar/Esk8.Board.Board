
#define LONGCLICK_MS    1000

void button_init()
{
  #ifdef USING_BUTTONS
  button0.setPressedHandler([](Button2 &btn)
  {
    EventsEnum e = EV_MOVING;
    vescdata.moving = true;
    DEBUGVAL(vescdata.moving);
    xQueueSendToFront(xEventQueue, &e, pdMS_TO_TICKS(10));
  });
  button0.setReleasedHandler([](Button2 &btn)
  {
    EventsEnum e = EV_STOPPED;
    vescdata.odometer = vescdata.odometer + 0.1;
    vescdata.moving = false;
    DEBUGVAL(vescdata.moving);
    xQueueSendToFront(xEventQueue, &e, pdMS_TO_TICKS(10));
  });
  button0.setLongClickHandler([](Button2 &btn)
  {
    // fake_vesc_online = true;
    // DEBUGVAL(fake_vesc_online);
  });
  #endif
}

void button_loop()
{
#ifdef USING_BUTTONS
  button0.loop();
#endif
}
//------------------------------------------------------------------
void light_init() {
  // light.initialise(PIXEL_PIN, NUM_PIXELS);
  // light.setAll(light.COLOUR_OFF);
}
