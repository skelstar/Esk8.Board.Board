
#pragma once

#include <Arduino.h>
#include <FsmManager.h>
#include <constants.h>
#include <printFormatStrings.h>
#include <TFT_eSPI.h>

namespace nsM5StackDisplayTask
{
  // prototypes
  const char *stateID(uint16_t id);

  TFT_eSPI tft = TFT_eSPI(LCD_HEIGHT, LCD_WIDTH);

  enum Trigger
  {
    TR_NO_EVENT = 0,
    TR_MOVING,
    TR_STOPPED,
    TR_BRAKING,
  };

  enum StateID
  {
    ST_READY = 0,
    ST_MOVING,
    ST_STOPPED,
    ST_BRAKING,
  };
  //----------------------------------------

  //----------------------------------------

  void stateReadyOnEnter();
  void stateMovingOnEnter();
  void stateStoppedOnEnter();
  void stateBrakingOnEnter();

  State stateReady(
      StateID::ST_READY,
      stateReadyOnEnter,
      NULL, NULL);

  State stateMoving(
      StateID::ST_MOVING,
      stateMovingOnEnter,
      NULL, NULL);

  State stateStopped(
      StateID::ST_STOPPED,
      stateStoppedOnEnter,
      NULL, NULL);

  State stateBraking(
      StateID::ST_BRAKING,
      stateBrakingOnEnter,
      NULL, NULL);
  //----------------------------------------

  Fsm fsm1(&stateReady);

  FsmManager<Trigger> fsm_mgr;

  void addTransitions()
  {
    fsm1.add_transition(&stateReady, &stateMoving, Trigger::TR_MOVING, NULL);
    fsm1.add_transition(&stateStopped, &stateMoving, Trigger::TR_MOVING, NULL);
    fsm1.add_transition(&stateMoving, &stateMoving, Trigger::TR_MOVING, NULL);
    fsm1.add_transition(&stateBraking, &stateMoving, Trigger::TR_MOVING, NULL); // edge

    fsm1.add_transition(&stateMoving, &stateStopped, Trigger::TR_STOPPED, NULL);
    fsm1.add_transition(&stateBraking, &stateStopped, Trigger::TR_STOPPED, NULL);

    fsm1.add_transition(&stateReady, &stateBraking, Trigger::TR_BRAKING, NULL);
    fsm1.add_transition(&stateStopped, &stateBraking, Trigger::TR_BRAKING, NULL);
    fsm1.add_transition(&stateBraking, &stateBraking, Trigger::TR_BRAKING, NULL);
    fsm1.add_transition(&stateMoving, &stateBraking, Trigger::TR_BRAKING, NULL);
  }

  void initFsm(bool print = false)
  {
    fsm_mgr.begin(&fsm1);
    fsm_mgr.setPrintStateCallback([](uint16_t id) {
      if (PRINT_DISP_FSM_STATE)
        Serial.printf(PRINT_FSM_STATE_FORMAT, "m5Stack", millis(), stateID(id));
    });
    fsm_mgr.setPrintTriggerCallback([](uint16_t ev) {
      if (PRINT_DISP_FSM_TRIGGER)
        Serial.printf(PRINT_FSM_TRIGGER_FORMAT, "m5Stack", millis(), "trigger(ev)");
    });

    addTransitions();
  }

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
    return "OUT OF RANGE: FSM::stateID()";
  }

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
    return "OUT OF RANGE: FSM::event()";
  }

  void drawCard(const char *text, uint32_t foreColour, uint32_t bgColour)
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

  void initTFT()
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

} // namespace