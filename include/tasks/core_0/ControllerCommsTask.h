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
  bool printRadioDetails = true,
       printRxFromController = false;

  elapsedMillis since_got_packet_from_controller = 0;

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
  void _initialise()
  {
    controllerQueue = createQueue<ControllerData>("(ControllerCommsTask) controllerQueue");
    vescDataQueue = createQueue<VescData>("(ControllerCommsTask) vescDataQueue");

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
      // if (vescDataQueue->payload.moving)
      //   Serial.printf("replied after %lums moving: %d\n", (unsigned long)since_got_packet_from_controller, vescDataQueue->payload.moving);
      // Serial.printf("replied after %lums\n", (unsigned long)since_got_packet_from_controller);
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
    ctrlrCommsTask.since_got_packet_from_controller = 0;

    if (type == Packet::CONTROL)
    {
      ControllerData controllerPacket = ctrlrCommsTask.controllerClient->read();

      sendPacket.id = controllerPacket.id;
      sendPacket.throttle = controllerPacket.throttle;
      sendPacket.txTime = controllerPacket.txTime;

      ctrlrCommsTask.controllerQueue->send(&sendPacket);

      if (ctrlrCommsTask.printRxFromController)
        ControllerData::print(sendPacket, "[controllerPacketAvailable_cb]-->");

      vTaskDelay(TICKS_5ms);
    }
  }
}