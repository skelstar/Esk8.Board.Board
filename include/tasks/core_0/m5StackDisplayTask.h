

namespace M5StackDisplay
{
  bool taskReady = false;

  enum Event
  {
    NO_EVENT = 0,
    MOVING,
    STOPPED,
  };

  const char *name(uint16_t ev)
  {
    switch (ev)
    {
    case NO_EVENT:
      return "NO_EVENT";
    case MOVING:
      return " MOVING ";
    case STOPPED:
      return "STOPPED";
    }
    return outOfRange("M5StackDisplay name()");
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
      // buttons
#define BTN_A_XPOS 65
#define BTN_B_XPOS LCD_WIDTH / 2
#define BTN_C_XPOS LCD_WIDTH - BTN_A_XPOS
      int POSs[] = {BTN_A_XPOS, BTN_B_XPOS, BTN_C_XPOS};
      const char *names[] = {"MOVE", "RESET", "-"};
      int size = 10,
          offset = 10,
          startY = LCD_HEIGHT - (size * 1.5 + offset),
          endY = LCD_HEIGHT - offset;
      tft.setTextSize(2);
      tft.setTextColor(TFT_WHITE, TFT_BLUE);

      for (int p = 0; p < 3; p++)
      {
        tft.fillTriangle(POSs[p] - size, startY, POSs[p], endY, POSs[p] + size, startY, TFT_WHITE);
        tft.drawString(names[p], POSs[p], startY - 15);
      }
      // ready card
      drawCard("READY", TFT_BLACK, TFT_LIGHTGREY);
    }
  } // namespace TFT

  void task(void *pvParameters)
  {
    Serial.printf("M5StackDisplay Task running on CORE_%d\n", xPortGetCoreID());

    TFT::init();

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
        case NO_EVENT:
          break;
        case MOVING:
          TFT::drawCard("MOVING", TFT_WHITE, TFT_DARKGREEN);
          break;
        case STOPPED:
          TFT::drawCard("STOPPED", TFT_WHITE, TFT_RED);
          break;
        }
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