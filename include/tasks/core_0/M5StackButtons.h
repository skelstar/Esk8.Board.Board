

namespace Buttons
{
  // bool taskReady = false;

  // void task(void *pvParameters)
  // {
  //   Serial.printf("[TASK] Buttons Task running on CORE_%d\n", xPortGetCoreID());

  //   button_init();
  //   primaryButtonInit();

  //   taskReady = true;

  //   while (true)
  //   {
  //     button0.loop();
  //     primaryButton.loop();

  //     buttonA.loop();
  //     if (sinceUpdatedButtonAValues > 1000 && buttonA.isPressedRaw())
  //     {
  //       sinceUpdatedButtonAValues = 0;
  //       long r = random(300);
  //       board_packet.batteryVoltage -= r / 1000.0;
  //       if (MOCK_MOVING_WITH_BUTTON == 1)
  //         mockMoving(buttonA.isPressedRaw());
  //     }

  //     buttonB.loop();
  //     buttonC.loop();

  //     vTaskDelay(10);
  //   }
  //   vTaskDelete(NULL);
  // }

  // void createTask(uint8_t core, uint8_t priority)
  // {
  //   xTaskCreatePinnedToCore(
  //       task,
  //       "ButtonsTask",
  //       5000,
  //       NULL,
  //       priority,
  //       NULL,
  //       core);
  // }
} // namespace Buttons