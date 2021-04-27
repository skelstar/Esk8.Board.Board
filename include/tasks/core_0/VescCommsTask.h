#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <VescData.h>
#include <vesc_comms.h>
#include <vesc_utils.h>

namespace nsVescCommsTask
{
  vesc_comms vesc;

  // prototypes
  VescData *get_vesc_values();
}

class VescCommsTask : public TaskBase
{
public:
  bool printWarnings = true;

private:
  // BatteryInfo Prototype;

  Queue1::Manager<ControllerData> *controllerQueue = nullptr;
  Queue1::Manager<VescData> *vescDataQueue = nullptr;

  VescData dummyVescData;

public:
  VescCommsTask(unsigned long p_doWorkInterval) : TaskBase("VescCommsTask", 5000, p_doWorkInterval)
  {
    _core = CORE_1;
    _priority = TASK_PRIORITY_3;
  }

private:
  void initialiseQueues()
  {
    controllerQueue = createQueue<ControllerData>("(VescCommsTask) controllerQueue");
    controllerQueue->read(); // clear the queue
    vescDataQueue = createQueue<VescData>("(VescCommsTask) vescDataQueue");
  }

  void initialise()
  {
  }

  bool timeToDoWork()
  {
    return true;
  }

  void doWork()
  {
    if (controllerQueue->hasValue())
    {
      ControllerData::print(controllerQueue->payload, "[VescCommsTask]");

      if (SEND_TO_VESC)
      {
        nsVescCommsTask::vesc.setNunchuckValues(
            /*x*/ 127,
            /*y*/ controllerQueue->payload.throttle,
            controllerQueue->payload.cruise_control,
            /*upper button*/ 0);
      }
      else
      {
        // return immediately
        dummyVescData.id = controllerQueue->payload.id;
        vescDataQueue->send(&dummyVescData);
      }
    }

    VescData *packet = nsVescCommsTask::get_vesc_values();
    if (packet != nullptr)
    {
      vescDataQueue->send(packet);
      VescData::print(*packet);
    }
  } // doWork
};

VescCommsTask vescCommsTask(PERIOD_10ms);

namespace nsVescCommsTask
{
  void task1(void *parameters)
  {
    vescCommsTask.task(parameters);
  }
  //-----------------------------------------------------------------------
  VescData *get_vesc_values()
  {
    uint8_t vesc_packet[PACKET_MAX_LENGTH];
    vesc_comms vesc;
    VescData *board_packet_r;

    bool success = vesc.fetch_packet(vesc_packet) > 0;

    if (!success)
      return nullptr;

    int32_t rpm_raw = vesc.get_rpm(vesc_packet);

    board_packet_r->batteryVoltage = vesc.get_voltage(vesc_packet);
    board_packet_r->moving = rpm_raw > RPM_AT_MOVING;

    board_packet_r->ampHours = vesc.get_amphours_discharged(vesc_packet) - initial_ampHours;
    board_packet_r->motorCurrent = vesc.get_motor_current(vesc_packet);
    board_packet_r->odometer = get_distance_in_meters(vesc.get_tachometer(vesc_packet)) - initial_odometer;
    board_packet_r->temp_mosfet = vesc.get_temp_mosfet(vesc_packet);

    return board_packet_r;
  }
}
