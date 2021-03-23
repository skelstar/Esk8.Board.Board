

namespace M5StackDisplay
{
  bool taskReady = false;

  enum StateID
  {
    ST_READY = 0,
    ST_MOVING,
    ST_STOPPED,
    ST_BRAKING,
  };
  const char *stateID(uint16_t id)
  {
    switch (id)
    {
    case ST_READY:
      return "ST_READY";
    case ST_MOVING:
      return "ST_MOVING";
    case ST_STOPPED:
      return "ST_STOPPED";
    case ST_BRAKING:
      return "ST_BRAKING";
    }
    return outOfRange("FSM::stateID()");
  }

  enum Trigger
  {
    TR_NO_EVENT = 0,
    TR_MOVING,
    TR_STOPPED,
    TR_BRAKING,
  };

  const char *trigger(uint16_t ev)
  {
    switch (ev)
    {
    case TR_MOVING:
      return "MOVING";
    case TR_STOPPED:
      return "STOPPED";
    case TR_BRAKING:
      return "TR_BRAKING";
    }
    return outOfRange("FSM::event()");
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
    FsmManager<M5StackDisplay::Trigger> fsm_mgr;

    State stateReady(
        StateID::ST_READY,
        [] {
          fsm_mgr.printState(StateID::ST_READY);
        },
        NULL, NULL);
    State stateMoving(
        StateID::ST_MOVING,
        [] {
          fsm_mgr.printState(StateID::ST_MOVING);
          // TODO read from local instead
          uint8_t t = controller.data.throttle;
          char thr[10];
          sprintf(thr, "%03d", t);
          TFT::drawCard(thr, TFT_WHITE, TFT_DARKGREEN);
        },
        NULL, NULL);
    State stateStopped(
        StateID::ST_STOPPED,
        [] {
          fsm_mgr.printState(StateID::ST_STOPPED);
          TFT::drawCard("STOPPED", TFT_WHITE, TFT_BLACK);
        },
        NULL, NULL);

    State stateBraking(
        StateID::ST_BRAKING,
        [] {
          fsm_mgr.printState(StateID::ST_BRAKING);
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
      fsm.add_transition(&stateBraking, &stateMoving, M5StackDisplay::TR_MOVING, NULL); // edge

      fsm.add_transition(&stateMoving, &stateStopped, M5StackDisplay::TR_STOPPED, NULL);
      fsm.add_transition(&stateBraking, &stateStopped, M5StackDisplay::TR_STOPPED, NULL);

      fsm.add_transition(&stateReady, &stateBraking, M5StackDisplay::TR_BRAKING, NULL);
      fsm.add_transition(&stateStopped, &stateBraking, M5StackDisplay::TR_BRAKING, NULL);
      fsm.add_transition(&stateBraking, &stateBraking, M5StackDisplay::TR_BRAKING, NULL);
      fsm.add_transition(&stateMoving, &stateBraking, M5StackDisplay::TR_BRAKING, NULL);
    }
  } // namespace FSM
  //======================================================================

  void task(void *pvParameters)
  {
    using namespace FSM;
    Serial.printf("M5StackDisplay Task running on CORE_%d\n", xPortGetCoreID());

    TFT::init();

    fsm_mgr.begin(&fsm);
    fsm_mgr.setPrintStateCallback([](uint16_t id) {
      if (PRINT_DISP_FSM_STATE)
        Serial.printf(PRINT_STATE_FORMAT, "m5Stack", M5StackDisplay::stateID(id));
    });
    fsm_mgr.setPrintTriggerCallback([](uint16_t ev) {
      if (PRINT_DISP_FSM_TRIGGER)
        Serial.printf(PRINT_sFSM_sTRIGGER_FORMAT, "m5Stack", trigger(ev));
    });

    FSM::addTransitions();

    taskReady = true;

    elapsedMillis since_checked_queues, since_debug;

    while (true)
    {
      if (since_checked_queues > 100)
      {
        since_checked_queues = 0;

        VescData *vesc = vescQueue->peek<VescData>(__func__);
        ControllerClass *ctrlr = ctrlrQueue->peek<ControllerClass>(__func__);

        if (vesc != nullptr)
        {
          if (vesc->moving && fsm_mgr.currentStateIs(StateID::ST_MOVING) == false)
            fsm_mgr.trigger(TR_MOVING);
          else if (vesc->moving == false && fsm_mgr.currentStateIs(StateID::ST_STOPPED) == false)
            fsm_mgr.trigger(TR_STOPPED);
        }
        else if (ctrlr != nullptr)
        {
          if (controller.throttleChanged())
          {
            if (controller.data.throttle == 127 && !fsm_mgr.currentStateIs(StateID::ST_STOPPED))
              fsm_mgr.trigger(TR_STOPPED);
            else if (controller.data.throttle > 127)
              fsm_mgr.trigger(TR_MOVING);
            else if (controller.data.throttle < 127)
              fsm_mgr.trigger(TR_BRAKING);
          }
        }
      }

      fsm_mgr.runMachine();

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