#pragma once

#include <Arduino.h>

QueueHandle_t xControllerQueueHandle;
QueueHandle_t xVescQueueHandle;
QueueHandle_t xSimplMessageQueue;

enum SimplMessage
{
  SIMPL_NONE = 0,
  SIMPL_TOGGLE_MOCK_MOVING_LOOP,
};

const char *getSimplMessage(int msg)
{
  switch (msg)
  {
  case SIMPL_NONE:
    return "SIMPL_NONE";
  case SIMPL_TOGGLE_MOCK_MOVING_LOOP:
    return "SIMPL_TOGGLE_MOCK_MOVING_LOOP";
  }
  return "OUT OF RANGE (getSimplMessage())";
}

class SimplMessageObj : public QueueBase
{
public:
  SimplMessage message = SimplMessage::SIMPL_NONE;

  SimplMessageObj() : QueueBase()
  {
  }

  static void print(SimplMessageObj obj, const char *preamble = nullptr)
  {
    Serial.printf("%s ", preamble != nullptr ? preamble : "[-]");
    Serial.printf("message: %d/%s ", obj.message, getSimplMessage(obj.message));
    Serial.printf("event_id: %lu ", obj.event_id);
    Serial.printf("\n");
  }
};