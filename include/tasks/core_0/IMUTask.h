#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <Wire.h>
#include <SparkFun_ADXL345.h>

#define IMU_TASK

namespace nsIMUTask
{
  // prototypes
  void _handleSimplMessage(SimplMessageObj obj);
}

class IMUTask : public TaskBase
{
public:
  bool printWarnings = true;

  enum BoardInclination
  {
    NONE = 0,
    FLAT,
    RAISED,
  };

private:
  Queue1::Manager<SimplMessageObj> *simplMsgQueue = nullptr;

  // #define ADXL_ADDR 0x20
  ADXL345 adxl = ADXL345();

  BoardInclination _inclination;

public:
  IMUTask() : TaskBase("IMUTask", 3000, PERIOD_100ms)
  {
    _core = CORE_0;
  }

private:
  void _initialise()
  {
    simplMsgQueue = createQueue<SimplMessageObj>("(IMUTask) simplMsgQueue");
    simplMsgQueue->payload.setGetMessageCallback(getSimplMessage);

    if (mux_I2C == nullptr)
      mux_I2C = xSemaphoreCreateMutex();

    // if (take(mux_I2C, TICKS_500ms))
    // {
    //   i2cScanner();
    //   give(mux_I2C);
    // }

    if (take(mux_I2C, TICKS_500ms))
    {
      adxl.powerOn();
      adxl.setRangeSetting(8);
      give(mux_I2C);
    }
    else
    {
      DEBUG("ERROR: could not initialise IMU (locked muxetx)");
    }
  }

  void doWork()
  {
    BoardInclination newInclination = _getInclination();

    if (newInclination != BoardInclination::NONE)
    {
      simplMsgQueue->payload.message = newInclination == RAISED
                                           ? SIMPL_BOARD_RAISED
                                           : SIMPL_BOARD_FLAT;
      simplMsgQueue->send(PRINT_THIS);
    }
  }

  void cleanup()
  {
    delete (simplMsgQueue);
  }

private:
  BoardInclination _getInclination(bool print = false)
  {
    int x, y, z;
    if (take(mux_I2C, TICKS_10ms))
    {
      adxl.readAccel(&x, &y, &z);
      give(mux_I2C);
    }

    BoardInclination newInclination = y > 80
                                          ? BoardInclination::RAISED
                                          : BoardInclination::FLAT;
    bool changed = _inclination != newInclination;

    _inclination = newInclination;

    if (print)
      Serial.printf("Accel: x=%d y=%d z=%d  \n", x, y, z);

    return changed
               ? newInclination
               : BoardInclination::NONE;
  }
};

IMUTask imuTask;

namespace nsIMUTask
{
  void task1(void *parameters)
  {
    imuTask.task(parameters);
  }
}
