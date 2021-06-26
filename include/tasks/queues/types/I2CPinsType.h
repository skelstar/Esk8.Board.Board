#pragma once

#include <Arduino.h>
#include <QueueBase.h>

class I2CPinsType : public QueueBase
{
public:
  uint16_t inputs = 0;
  uint16_t outputs = 0;

  void print(const char *preamble = nullptr)
  {
    Serial.printf("%s ", preamble != nullptr ? preamble : "[-]");
    Serial.printf("Inputs: 0x%02x Outputs: 0x%02x ", inputs, outputs);
    Serial.printf("event_id: %lu ", event_id);
    Serial.printf("\n");
  }
};