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
      : TaskBase("ControllerCommsTask", 5000, p_doWorkInterval)
  {
    _core = CORE_1;
    _priority = TASK_PRIORITY_4;
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
    vTaskDelay(1);
    controllerClient->update();
  }
};

ControllerCommsTask controllerCommsTask(PERIOD_50ms);

namespace nsControllerCommsTask
{
  void task1(void *parameters)
  {
    controllerCommsTask.task(parameters);
  }

  ControllerData sendPacket;

  void controllerPacketAvailable_cb(uint16_t from_id, uint8_t type)
  {
    Serial.printf("packet available\n");
    if (type == Packet::CONTROL)
    {
      ControllerData packet = controllerCommsTask.controllerClient->read();

      sendPacket.id = packet.id;
      sendPacket.throttle = packet.throttle;

      controllerCommsTask.controllerQueue->send(&sendPacket);
      ControllerData::print(sendPacket, "---------------\ncontrollerPacketAvailable_cb: ");
      vTaskDelay(TICKS_100ms);
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