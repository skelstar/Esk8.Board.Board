

namespace M5StackDisplay
{
  bool taskReady = false;

  namespace TFT
  {
    void init()
    {
      tft.init();
      tft.setRotation(1);
      tft.fillScreen(TFT_BLUE);
      tft.setTextColor(TFT_WHITE, TFT_BLUE);
      tft.setTextSize(3);
      tft.setTextDatum(MC_DATUM);
      // ready
      tft.drawString("READY", LCD_WIDTH / 2, LCD_HEIGHT / 2);
      // buttons
#define BTN_A_POS 65
#define BTN_B_POS LCD_WIDTH / 2
#define BTN_C_POS LCD_WIDTH - BTN_A_POS
      int POSs[] = {BTN_A_POS, BTN_B_POS, BTN_C_POS};
      const char *names[] = {"MOVE", "RESET", "-"};
      int size = 10,
          offset = 10,
          startY = LCD_HEIGHT - (size * 1.5 + offset),
          endY = LCD_HEIGHT - offset;
      tft.setTextSize(2);
      for (int p = 0; p < 3; p++)
      {
        tft.fillTriangle(POSs[p] - size, startY, POSs[p], endY, POSs[p] + size, startY, TFT_WHITE);
        tft.drawString(names[p], POSs[p], startY - 15);
      }
    }
  } // namespace TFT

  void task(void *pvParameters)
  {
    Serial.printf("M5StackDisplay Task running on CORE_%d\n", xPortGetCoreID());

    TFT::init();

    taskReady = true;

    while (true)
    {
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