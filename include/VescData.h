#ifndef VescData_h
#define VescData_h

#pragma once
#include <elapsedMillis.h>

#include "Arduino.h"

enum ReasonType
{
  BOARD_STOPPED,
  BOARD_MOVING,
  FIRST_PACKET,
  LAST_WILL,
  REQUESTED,
  VESC_OFFLINE,
  RESPONSE,
};

enum PacketType
{
  CONTROL,
  CONFIG,
};

enum CommandType
{
  NONE,
  RESET,
};

class VescData
{
public:
  unsigned long id;
  float batteryVoltage;
  bool moving;
  float ampHours;
  float motorCurrent;
  float odometer; // in kilometers
  bool vescOnline;
  ReasonType reason;
  CommandType command;
  float version;

  const char *getSummary()
  {
    char buff[50];
    sprintf(buff, "id: %lu moving: %d", id, moving);
    return buff;
  }
};

class ControllerData
{
public:
  unsigned long id;
  uint8_t throttle;
  bool cruise_control;
  uint8_t command;

  const char *getSummary()
  {
    char buff[50];
    sprintf(buff, "id: %lu throttle: %d", id, throttle);
    return buff;
  }
};

class ControllerConfig
{
public:
  unsigned long id;
  uint16_t send_interval;
};

class ControllerClass
{
public:
  ControllerData data;
  ControllerConfig config;
  elapsedMillis sinceLastPacket;

  void save(ControllerData latest)
  {
    _prev = data;
    data = latest;
    sinceLastPacket = 0;
  }

  void save(ControllerConfig latestConfig)
  {
    config = latestConfig;
    sinceLastPacket = 0;
  }

  uint16_t missedPackets()
  {
    return data.id > 0
               ? (data.id - _prev.id) - 1
               : 0;
  }

  bool throttleChanged()
  {
    return data.throttle != _prev.throttle;
  }

  bool hasTimedout(elapsedMillis lastPacketTime)
  {
    return config.send_interval > 0 &&
           lastPacketTime > config.send_interval + 100;
  }

private:
  ControllerData _prev;
};

#endif
