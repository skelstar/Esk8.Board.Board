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
  Queue1::Manager<VescData> *vescDataQueue = nullptr;

  // ControllerData controllerPacket;

private:
public:
  ControllerCommsTask()
      : TaskBase("ControllerCommsTask", 5000, PERIOD_50ms)
  {
    _core = CORE_1;
    _priority = TASK_PRIORITY_4;
  }

private:
  void initialiseQueues()
  {
    controllerQueue = createQueue<ControllerData>("(ControllerCommsTask) controllerQueue");
    vescDataQueue = createQueue<VescData>("(ControllerCommsTask) vescDataQueue");
  }

  void initialise()
  {
    if (mux_SPI == nullptr)
      mux_SPI = xSemaphoreCreateMutex();

    vTaskDelay(TICKS_100ms);

    if (take(mux_SPI, TICKS_1s))
    {
      nrf24.begin(&radio, &network, COMMS_BOARD, nullptr, false, printRadioDetails);
      give(mux_SPI);
    }
    else
    {
      Serial.printf("TASK: ControllerCommsTask unable to init radio (take mux)\n");
    }

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

    // reply to controller on queue
    if (vescDataQueue->hasValue())
    {
      // send to controller
      vescDataQueue->payload.version = VERSION;

      controllerClient->sendTo(Packet::CONTROL, vescDataQueue->payload);
      VescData::print(vescDataQueue->payload, "[ControllerCommsTask]:reply");
    }
  }

  void cleanup()
  {
    delete (controllerClient);
    delete (controllerQueue);
    delete (vescDataQueue);
  }
};

ControllerCommsTask ctrlrCommsTask;

namespace nsControllerCommsTask
{
  void task1(void *parameters)
  {
    ctrlrCommsTask.task(parameters);
  }

  ControllerData sendPacket;

  void controllerPacketAvailable_cb(uint16_t from_id, uint8_t type)
  {
    if (type == Packet::CONTROL)
    {
      ControllerData packet = ctrlrCommsTask.controllerClient->read();

      sendPacket.id = packet.id;
      sendPacket.throttle = packet.throttle;
      sendPacket.txTime = packet.txTime;

      ctrlrCommsTask.controllerQueue->send(&sendPacket);
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