#pragma once

#include <QueueBase.h>

class ControllerClass : public QueueBase
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
    // DEBUGVAL(config.send_interval, lastPacketTime);
    return config.send_interval > 0 &&
           lastPacketTime > config.send_interval + 100;
  }

private:
  ControllerData _prev;
};
