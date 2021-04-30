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
  Queue1::Manager<VescData> *vescReadDataQueue = nullptr;

  VescData *vescData;

public:
  VescCommsTask() : TaskBase("VescCommsTask", 5000, PERIOD_50ms)
  {
    _core = CORE_0;
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
    vescData = new VescData();
  }

  bool timeToDoWork()
  {
    return true;
  }

#define IGNORE_X_AXIS 127

  void doWork()
  {
    if (vescDataQueue->hasValue())
    {
      VescData::print(vescDataQueue->payload, "[VescCommsTask] vescDataQueue:read ");
      vescData->moving = vescDataQueue->payload.moving;
    }

    if (controllerQueue->hasValue())
    {
      ControllerData::print(controllerQueue->payload, "[VescCommsTask] ControllerQueue:read ");

      vescData->id = controllerQueue->payload.id;
      vescData->txTime = controllerQueue->payload.txTime;
      vescData->version = VERSION;

      if (SEND_TO_VESC)
      {
        nsVescCommsTask::vesc.setNunchuckValues(
            IGNORE_X_AXIS,
            /*y*/ controllerQueue->payload.throttle,
            controllerQueue->payload.cruise_control,
            /*upper button*/ 0);
      }
      else
      {
        // reply immediately
        vescDataQueue->send(vescData);
        VescData::print(*vescData, "[VescCommsTask]:reply");
      }
    }

    if (SEND_TO_VESC == 1)
    {
      vescData = nsVescCommsTask::get_vesc_values();
      if (vescData != nullptr)
      {
        vescDataQueue->send(vescData);
      }
    }
  } // doWork

  void cleanup()
  {
    delete (controllerQueue);
    delete (vescDataQueue);
    delete (vescData);
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
