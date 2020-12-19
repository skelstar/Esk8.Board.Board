

namespace Buttons
{
  void task(void *pvParameters)
  {
    Serial.printf("Buttons Task running on CORE_%d\n", xPortGetCoreID());

    button_init();
    primaryButtonInit();

    while (true)
    {
      button0.loop();
      primaryButton.loop();

      buttonA.loop();
      if (sinceUpdatedButtonAValues > 1000 && buttonA.isPressed())
      {
        sinceUpdatedButtonAValues = 0;
        long r = random(300);
        board_packet.batteryVoltage -= r / 1000.0;
        if (MOCK_MOVING_WITH_BUTTON == 1)
          mockMoving(buttonA.isPressed());
      }

      buttonB.loop();
      buttonC.loop();

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }

  void createTask(uint8_t core, uint8_t priority)
  {
    xTaskCreatePinnedToCore(
        task,
        "ButtonsTask",
        5000,
        NULL,
        priority,
        NULL,
        core);
  }
} // namespace Buttons