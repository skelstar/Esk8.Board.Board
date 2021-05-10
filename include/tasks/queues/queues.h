#pragma once

#include <Arduino.h>

QueueHandle_t xControllerQueueHandle;
QueueHandle_t xVescQueueHandle;
QueueHandle_t xSimplMessageQueue;

enum SimplMessage
{
  SIMPL_NONE = 0,
  SIMPL_TOGGLE_MOCK_MOVING_LOOP,
  SIMPL_HEADLIGHT_ON,
  SIMPL_HEADLIGHT_OFF,
  I2C_INPUT_7_PRESSED,
  SIMPL_BOARD_FLAT,
  SIMPL_BOARD_RAISED,
};

const char *getSimplMessage(int msg)
{
  switch (msg)
  {
  case SIMPL_NONE:
    return "SIMPL_NONE";
  case SIMPL_TOGGLE_MOCK_MOVING_LOOP:
    return "SIMPL_TOGGLE_MOCK_MOVING_LOOP";
  case SIMPL_HEADLIGHT_ON:
    return "SIMPL_HEADLIGHT_ON";
  case SIMPL_HEADLIGHT_OFF:
    return "SIMPL_HEADLIGHT_OFF";
  case SIMPL_BOARD_FLAT:
    return "SIMPL_BOARD_FLAT";
  case SIMPL_BOARD_RAISED:
    return "SIMPL_BOARD_RAISED";
  }
  return "OUT OF RANGE (getSimplMessage())";
}
