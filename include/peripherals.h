
#define LONGCLICK_MS    1000

void button_init()
{
  // btn1.setPressedHandler([](Button2 &b) {
  //   Serial.printf("btn1.setPressedHandler()\n");
  // });
  // btn1.setReleasedHandler([](Button2 &b) {
  //   Serial.printf("btn1.setReleasedHandler()\n");
  // });
  // btn1.setClickHandler([](Button2 &b) {
  //   Serial.printf("btn1.setClickHandler()\n");

    // old_packet.id = controller_packet.id - 5;
    // fsm.trigger(EV_MISSED_CONTROLLER_PACKET);
    // fsm.run_machine();
  // });
  // btn1.setLongClickHandler([](Button2 &b) {
  //   Serial.printf("btn1.setLongClickHandler([](Button2 &b)\n");
  // });
  // btn1.setDoubleClickHandler([](Button2 &b) {
  //   Serial.printf("btn1.setDoubleClickHandler([](Button2 &b)\n");
  // });
  // btn1.setTripleClickHandler([](Button2 &b) {
  //   Serial.printf("btn1.setTripleClickHandler([](Button2 &b)\n");
  // });
}

void button_loop()
{
  button0.loop();
}
//------------------------------------------------------------------
void initialiseLeds() {
  DEBUG("Initialising LEDs (red)\n");
  FastLED.addLeds<WS2812B, PIXEL_PIN, GRB>(strip, NUM_PIXELS);
  FastLED.setBrightness(50);
  allLedsOn(COLOUR_RED);
  FastLED.show();
}
