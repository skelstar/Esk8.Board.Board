#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <VescData.h>
#include <vesc_comms.h>
#include <vesc_utils.h>

#define VESC_UART_BAUDRATE 115200

namespace nsVescCommsTask
{
  vesc_comms vesc;

  // prototypes
  bool get_vesc_values(VescData &vescData);
}

class VescCommsTask : public TaskBase
{
public:
  bool printWarnings = true,
       printReadFromVesc = false,
       printSentToVesc = false,
       printQueues = false;

private:
  // BatteryInfo Prototype;

  Queue1::Manager<ControllerData> *controllerQueue = nullptr;
  Queue1::Manager<VescData> *vescDataQueue = nullptr;

  bool mockMovingLoop = false;
  unsigned long lastControllerPacketId = 0;

  elapsedMillis sinceGotVesc, sinceHandledControllerPacket;
  const ulong mockMovinginterval = 5000;
  unsigned long whenGotVesc = 0, _sendInterval = 200;

public:
  VescCommsTask() : TaskBase("VescCommsTask", 5000)
  {
    _core = CORE_0;
  }

private:
  void _initialise()
  {
    controllerQueue = createQueue<ControllerData>("(VescCommsTask) controllerQueue");
    controllerQueue->read(); // clear the queue

    vescDataQueue = createQueue<VescData>("(VescCommsTask) vescDataQueue");
    vescDataQueue->printMissedPacket = false;

    nsVescCommsTask::vesc.init(VESC_UART_BAUDRATE);
  }

  void doWork()
  {
    if (controllerQueue->hasValue(printQueues))
      _handleControllerPacket(controllerQueue->payload);

    if (sinceGotVesc > _sendInterval)
      _updateDataFromVesc();
  }

  void cleanup()
  {
    delete (controllerQueue);
    delete (vescDataQueue);
  }

  void _updateDataFromVesc()
  {
    sinceGotVesc = 0;

    if (SEND_TO_VESC == 0)
      return;

    // half the packet are successful (right length), speed/interval makes no difference
    bool success = nsVescCommsTask::get_vesc_values(vescDataQueue->payload);

    // Serial.printf("------------------------------\n");
    if (printReadFromVesc)
      vescDataQueue->payload.print(_name, __func__);
  }

#define GET_VESC_TIME_BEFORE_CONTROLLER_PACKET_MS 50

  // - maps data to vescData
  // - send throttle (vescData) to VESC
  // - send to queue (with updated values)
  void _handleControllerPacket(ControllerData &packet)
  {
    sinceHandledControllerPacket = 0;
    _sendInterval = packet.sendInterval;
    // make sure gets from Vesc 100ms before controller packet arrives
    sinceGotVesc = GET_VESC_TIME_BEFORE_CONTROLLER_PACKET_MS;

    if (SEND_TO_VESC == 1)
    {
#define IGNORE_X_AXIS 127
#define IGNORE_UPPER_BUTTON 0
      // sends throttle to VESC
      nsVescCommsTask::vesc.setNunchuckValues(IGNORE_X_AXIS, /*y*/ packet.throttle, /*cruise*/ packet.cruise_control, IGNORE_UPPER_BUTTON);

      if (printSentToVesc)
        packet.print("[Sending to VESC]");
    }
    else
    {
      _mockMoving(packet);
    }

    vescDataQueue->payload.id = packet.id;
    vescDataQueue->payload.txTime = packet.txTime;
    vescDataQueue->payload.version = VERSION;

    vescDataQueue->sendPayload(printQueues); // should contain updated data from Vesc
  }

  void _mockMoving(ControllerData payload)
  {
    // if (mockMovingLoop && sinceLastMock > mockMovinginterval)
    // {
    //   sinceLastMock = 0;
    //   vescDataQueue->payload.moving = !vescDataQueue->payload.moving;
    // }
  }
};

VescCommsTask vescCommsTask;

namespace nsVescCommsTask
{
  void task1(void *parameters)
  {
    vescCommsTask.task(parameters);
  }
  //-----------------------------------------------------------------------
  bool get_vesc_values(VescData &vescData)
  {
    uint8_t vesc_packet[PACKET_MAX_LENGTH];

    int numBytes = vesc.fetch_packet(vesc_packet);

    if (numBytes == 0 || numBytes != 70)
      return false;

    if (vesc.get_voltage(vesc_packet) < 5.0)
      return false;

    int32_t rpm_raw = vesc.get_rpm(vesc_packet);

    vescData.batteryVoltage = vesc.get_voltage(vesc_packet);
    vescData.moving = rpm_raw > RPM_AT_MOVING;
    // Serial.printf("numBytes: %d rpm: %d, batt volts: %.1fV\n", numBytes, rpm_raw, vescData.batteryVoltage);
    // TODO initial_amphours etc
    vescData.ampHours = vesc.get_amphours_discharged(vesc_packet) - initial_ampHours;
    vescData.motorCurrent = vesc.get_motor_current(vesc_packet);
    vescData.odometer = get_distance_in_meters(vesc.get_tachometer(vesc_packet)) - initial_odometer;
    vescData.temp_mosfet = vesc.get_temp_mosfet(vesc_packet);

    return true;
  }
}
