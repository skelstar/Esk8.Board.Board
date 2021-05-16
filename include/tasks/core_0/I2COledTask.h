#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <U8g2lib.h>
#include <Wire.h>

#define I2COLED_TASK

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define OLED_RESET -1       //4        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

namespace nsI2COledTask
{
  // prototypes
  void _handleSimplMessage(SimplMessageObj obj);
}

class I2COledTask : public TaskBase
{
public:
  bool printWarnings = true;

private:
  Queue1::Manager<SimplMessageObj> *simplMsgQueue = nullptr;

public:
  I2COledTask() : TaskBase("I2COledTask", 3000, PERIOD_100ms)
  {
    _core = CORE_0;
  }

private:
  void _initialise()
  {
    simplMsgQueue = createQueue<SimplMessageObj>("(I2COledTask) simplMsgQueue");
    simplMsgQueue->payload.setGetMessageCallback(getSimplMessage);

    if (mux_I2C == nullptr)
      mux_I2C = xSemaphoreCreateMutex();

    if (take(mux_I2C, TICKS_500ms))
    {
      u8g2.setI2CAddress(SCREEN_ADDRESS);
      u8g2.begin();

      u8g2.setFont(u8g2_font_ncenB14_tr);
      u8g2.drawStr(0, 24, "Hello World!");
      //   if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
      //   {
      //     Serial.println(F("SSD1306 allocation failed"));
      //     for (;;)
      //       ; // Don't proceed, loop forever
      //   }

      //   vTaskDelay(TICKS_2s); // Pause for 2 seconds

      //   // Clear the buffer
      //   display.clearDisplay();

      //   display.setCursor(0, 0);
      //   display.println(F("ready"));
      //   display.display();

      give(mux_I2C);
    }
    else
    {
      DEBUG("ERROR: could not initialise I2COled (locked mutex)");
    }
  }

  void doWork()
  {
  }

  void cleanup()
  {
    delete (simplMsgQueue);
  }

private:
};

I2COledTask i2cOledTask;

namespace nsI2COledTask
{
  void task1(void *parameters)
  {
    i2cOledTask.task(parameters);
  }
}
