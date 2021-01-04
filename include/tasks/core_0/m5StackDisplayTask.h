

namespace M5StackDisplay
{
  bool taskReady = false;

  namespace TFT
  {
    void drawCard(const char *text, uint32_t foreColour = TFT_WHITE, uint32_t bgColour = TFT_BLUE)
    {
      int cardYOffset = 20,
          cardWidth = 200,
          cardHeight = 80,
          startx = LCD_WIDTH / 2 - cardWidth / 2,
          starty = LCD_HEIGHT / 2 - cardHeight / 2 - cardYOffset;
      tft.fillRect(startx + 5, starty + 5, cardWidth, cardHeight, TFT_BLACK); // shadow
      tft.fillRect(startx, starty, cardWidth, cardHeight, bgColour);          // foreground
      tft.setTextColor(foreColour, bgColour);
      tft.setTextSize(3);
      tft.setTextDatum(MC_DATUM);
      tft.drawString(text, LCD_WIDTH / 2, LCD_HEIGHT / 2 - cardYOffset);
    }

    void init()
    {
      tft.init();
      tft.setRotation(1);
      tft.fillScreen(TFT_BLUE);
      tft.setTextDatum(MC_DATUM);
      // buttons
      int position[] = {BTN_A_XPOS, BTN_B_XPOS, BTN_C_XPOS};
      const char *names[] = {"MOVE", "RESET", "-"};
      int size = 10,
          offset = 10,
          startY = LCD_HEIGHT - (size * 1.5 + offset),
          endY = LCD_HEIGHT - offset;
      tft.setTextSize(2);
      tft.setTextColor(TFT_WHITE, TFT_BLUE);

      for (int p = 0; p < 3; p++)
      {
        tft.fillTriangle(position[p] - size, startY, position[p], endY, position[p] + size, startY, TFT_WHITE);
        tft.drawString(names[p], position[p], startY - 15);
      }
      // ready card
      drawCard("READY", TFT_BLACK, TFT_LIGHTGREY);
    }
  } // namespace TFT

  namespace FSM
  {
    FsmManager<M5StackDisplay::Trigger> m5StackFsm;

    State stateReady(
        [] {
          m5StackFsm.printState(StateID::ST_READY);
        },
        NULL, NULL);
    State stateMoving(
        [] {
          m5StackFsm.printState(StateID::ST_MOVING);
          TFT::drawCard("MOVING", TFT_WHITE, TFT_DARKGREEN);
        },
        NULL, NULL);
    State stateStopped(
        [] {
          m5StackFsm.printState(StateID::ST_STOPPED);
          TFT::drawCard("STOPPED", TFT_WHITE, TFT_RED);
        },
        NULL, NULL);

    Fsm fsm(&stateReady);

    void addTransitions()
    {
      fsm.add_transition(&stateReady, &stateMoving, M5StackDisplay::TR_MOVING, NULL);
      fsm.add_transition(&stateMoving, &stateStopped, M5StackDisplay::TR_STOPPED, NULL);
    }
  } // namespace FSM
  //-----------------------------------------------------------
  void task(void *pvParameters)
  {
    using namespace FSM;
    Serial.printf("M5StackDisplay Task running on CORE_%d\n", xPortGetCoreID());

    TFT::init();

    m5StackFsm.begin(&fsm);
    m5StackFsm.setPrintStateCallback([](uint16_t id) {
      if (PRINT_DISP_FSM_STATE)
        Serial.printf(PRINT_STATE_FORMAT, "m5Stack", M5StackDisplay::stateID(id));
    });
    m5StackFsm.setPrintTriggerCallback([](uint16_t ev) {
      if (PRINT_DISP_FSM_TRIGGER)
        Serial.printf(PRINT_sFSM_sTRIGGER_FORMAT, "m5Stack", trigger(ev));
    });

    FSM::addTransitions();

    taskReady = true;

    elapsedMillis sinceCheckedQueue;

    while (true)
    {
      if (sinceCheckedQueue > 10)
      {
        sinceCheckedQueue = 0;

        uint16_t ev = displayQueue->read<uint16_t>();
        switch (ev)
        {
        case M5StackDisplay::Q_MOVING:
          FSM::m5StackFsm.trigger(M5StackDisplay::TR_MOVING);
          break;
        case M5StackDisplay::Q_STOPPED:
          FSM::m5StackFsm.trigger(M5StackDisplay::TR_STOPPED);
          break;
        }
      }
      m5StackFsm.runMachine();

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }

  void createTask(uint8_t core, uint8_t priority)
  {
    xTaskCreatePinnedToCore(
        task,
        "M5StackDisplayTask",
        5000,
        NULL,
        priority,
        NULL,
        core);
  }
} // namespace M5StackDisplay