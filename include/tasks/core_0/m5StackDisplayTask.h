

namespace M5StackDisplay
{
  bool taskReady = false;

  Trigger mapToTrigger(uint16_t ev)
  {
    switch (ev)
    {
    case Q_MOVING:
    case Q_RL_MOVING:
      return TR_MOVING;
    case Q_STOPPED:
      return TR_STOPPED;
    case Q_RL_STOPPING:
      return TR_STOPPING;
    }
    return TR_NO_EVENT;
  }

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

    enum StateId
    {
      READY = 0,
      MOVING,
      STOPPED,
      STOPPING,
    };

    State stateReady(
        StateId::READY,
        [] {
          m5StackFsm.printState(StateID::ST_READY);
        },
        NULL, NULL);
    State stateMoving(
        StateId::MOVING,
        [] {
          m5StackFsm.printState(StateID::ST_MOVING);
          uint8_t t = controller.data.throttle;
          char thr[10];
          sprintf(thr, "%03d", t);
          TFT::drawCard(thr, TFT_WHITE, TFT_DARKGREEN);
        },
        NULL, NULL);
    State stateStopped(
        StateId::STOPPED,
        [] {
          m5StackFsm.printState(StateID::ST_STOPPED);
          TFT::drawCard("STOPPED", TFT_WHITE, TFT_BLACK);
        },
        NULL, NULL);

    State stateStopping(
        StateId::STOPPING,
        [] {
          m5StackFsm.printState(StateID::ST_STOPPING);
          uint8_t t = controller.data.throttle;
          char thr[10];
          sprintf(thr, "%03d", t);
          TFT::drawCard(thr, TFT_WHITE, TFT_RED);
        },
        NULL, NULL);

    Fsm fsm(&stateReady);

    void addTransitions()
    {
      fsm.add_transition(&stateReady, &stateMoving, M5StackDisplay::TR_MOVING, NULL);
      fsm.add_transition(&stateStopped, &stateMoving, M5StackDisplay::TR_MOVING, NULL);
      fsm.add_transition(&stateMoving, &stateMoving, M5StackDisplay::TR_MOVING, NULL);
      fsm.add_transition(&stateStopping, &stateMoving, M5StackDisplay::TR_MOVING, NULL);

      fsm.add_transition(&stateMoving, &stateStopped, M5StackDisplay::TR_STOPPED, NULL);
      fsm.add_transition(&stateStopping, &stateStopped, M5StackDisplay::TR_STOPPED, NULL);

      fsm.add_transition(&stateReady, &stateStopping, M5StackDisplay::TR_STOPPING, NULL);
      fsm.add_transition(&stateStopped, &stateStopping, M5StackDisplay::TR_STOPPING, NULL);
      fsm.add_transition(&stateStopping, &stateStopping, M5StackDisplay::TR_STOPPING, NULL);
      fsm.add_transition(&stateMoving, &stateStopping, M5StackDisplay::TR_STOPPING, NULL);
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

    elapsedMillis sinceCheckedQueue, since_debug;

    while (true)
    {
      if (sinceCheckedQueue > 10)
      {
        sinceCheckedQueue = 0;

        VescData *vesc = vescQueue->peek<VescData>(__func__);

        if (vesc != nullptr)
        {
          if (vesc->moving && m5StackFsm.currentStateIs(StateId::MOVING) == false)
            fsm.trigger(TR_MOVING);
          else if (vesc->moving == false && m5StackFsm.currentStateIs(StateId::STOPPED) == false)
            fsm.trigger(TR_STOPPED);
        }
      }
      m5StackFsm.runMachine();

      if (since_debug > 3000)
      {
        since_debug = 0;
        Serial.printf("m5StackDisplayTask\n");
      }

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