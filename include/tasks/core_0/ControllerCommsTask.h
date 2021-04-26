#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <VescData.h>

NRF24L01Lib nrf24;
RF24 radio(SPI_CE, SPI_CS, RF24_SPI_SPEED);
RF24Network network(radio);

namespace nsControllerCommsTask
{
  ControllerData *controller_packet;

  void controllerPacketAvailable_cb(uint16_t from_id, uint8_t type);
}

class ControllerCommsTask : public TaskBase
{
public:
  bool printRadioDetails = true;

  GenericClient<VescData, ControllerData> *controllerClient;
  Queue1::Manager<ControllerData> *controllerQueue = nullptr;

private:
public:
  ControllerCommsTask(unsigned long p_doWorkInterval)
      : TaskBase("ControllerCommsTask", 3000, p_doWorkInterval)
  {
    _core = CORE_1;
    _priority = TASK_PRIORITY_3;
  }

private:
  void initialiseQueues()
  {
    controllerQueue = createQueue<ControllerData>("(ControllerCommsTask) controllerQueue");
  }

  void initialise()
  {
    if (mux_SPI == nullptr)
      mux_SPI = xSemaphoreCreateMutex();

    nrf24.begin(&radio, &network, COMMS_BOARD, nullptr, false, printRadioDetails);

    controllerClient = new GenericClient<VescData, ControllerData>(COMMS_CONTROLLER);
    controllerClient->begin(&network, nsControllerCommsTask::controllerPacketAvailable_cb, mux_SPI);
  }

  bool timeToDoWork()
  {
    return true;
  }

  void doWork()
  {
    controllerClient->update();
  }

  void lowerFunc()
  {
  }
};

ControllerCommsTask controllerCommsTask(PERIOD_50ms);

namespace nsControllerCommsTask
{
  void task1(void *parameters)
  {
    controllerCommsTask.task(parameters);
  }

  void controllerPacketAvailable_cb(uint16_t from_id, uint8_t type)
  {
    // sinceLastControllerPacket = 0;

    if (type == Packet::CONTROL)
    {
      ControllerData packet = controllerCommsTask.controllerClient->read();
      Serial.printf("Comms: id=%d\n", packet.id);
      // ControllerData::print(packet, "[COMMS]");

      //   controller.save(data);

      // if (SEND_TO_VESC)
      //   send_to_vesc(controller.data.throttle, /*cruise*/ controller.data.cruise_control);

      //   Serial.printf("rx CONTROL id: %lu | ", controller.data.id);

      //   ctrlrQueue->send(&controller);

      //   board_packet.id = controller.data.id;
      //   board_packet.reason = ReasonType::RESPONSE;

      //   sendPacketToController();

      //   if (PRINT_THROTTLE && controller.throttleChanged())
      //   {
      //     DEBUGVAL(controller.data.id, controller.data.throttle, controller.data.cruise_control);
      //   }
    }
    else if (type == Packet::CONFIG)
    {
      //   ControllerConfig config = controllerClient->readAlt<ControllerConfig>();
      //   controller.save(config);

      //   Serial.printf("rx CONFIG id: %lu | ", controller.data.id);

      //   board_packet.id = controller.config.id;
      //   board_packet.reason = ReasonType::CONFIG_RESPONSE;

      //   sendPacketToController();
      // }
      // else
      // {
      //   Serial.printf("unknown packet from controller: %d\n", type);
      // }
    }
  }
}