
#pragma once

#include <tasks/queues/queues.h>

#include <QueueManager.h>

template <typename T>
Queue1::Manager<T> *createQueue(const char *name, TickType_t ticks = TICKS_5ms)
{
  if (std::is_same<T, ControllerData>::value)
  {
    return new Queue1::Manager<T>(xControllerQueueHandle, TICKS_5ms, name);
  }
  if (std::is_same<T, VescData>::value)
  {
    return new Queue1::Manager<T>(xVescQueueHandle, TICKS_5ms, name);
  }
  if (std::is_same<T, I2CPinsType>::value)
  {
    return new Queue1::Manager<T>(xI2CPinsQueue, TICKS_5ms, name);
  }
  Serial.printf("ERROR: (Manager::create) a queue has not been created for this type (%s)\n", name);
  return nullptr;
}
