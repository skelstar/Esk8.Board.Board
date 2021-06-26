#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <VescData.h>

#define CONTROLLERCOMMS_TASK

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

  GenericClient<VescData, ControllerData> *controllerClient;

  Queue1::Manager<ControllerData> *controllerQueue = nullptr;
  Queue1::Manager<VescData> *vescDataQueue = nullptr;

  // ControllerData controllerPacket;

private:
  uint32_t _numPacketsSentToController = 0;

public:
  ControllerCommsTask()
      : TaskBase("ControllerCommsTask", 5000)
  {
    _core = CORE_1;
  }

private:
  void _initialise()
  {
    controllerQueue = createQueue<ControllerData>("(ControllerCommsTask) controllerQueue");
    vescDataQueue = createQueue<VescData>("(ControllerCommsTask) vescDataQueue");
    vescDataQueue->printMissedPacket = false; // seems to only miss one, but often

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
      _handleVescData(vescDataQueue->payload);
  }

  void cleanup()
  {
    delete (controllerClient);
    delete (controllerQueue);
    delete (vescDataQueue);
  }

  void _handleVescData(VescData &payload)
  {
    // send to controller
    payload.version = VERSION;
    payload.reason = _getReasonForSending();

    Serial.printf("[TASK]ControllerCommsTask _handleVescData() event_id: %lu \n", payload.event_id);

    bool success = controllerClient->sendTo(Packet::CONTROL, payload);

    if (success)
      _numPacketsSentToController++;
  }

  //-----------------------------
  ReasonType _getReasonForSending()
  {
    if (_numPacketsSentToController == 0)
      return ReasonType::FIRST_PACKET;
    return ReasonType::RESPONSE;
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
      // TODO try a local copy that updates OG using pointer
      ControllerData clientData = ctrlrCommsTask.controllerClient->read();
      ctrlrCommsTask.controllerQueue->payload.id = clientData.id;
      ctrlrCommsTask.controllerQueue->payload.cruise_control = clientData.cruise_control;
      ctrlrCommsTask.controllerQueue->payload.throttle = clientData.throttle;
      ctrlrCommsTask.controllerQueue->payload.txTime = clientData.txTime;
      ctrlrCommsTask.controllerQueue->payload.sendInterval = clientData.sendInterval;

      ctrlrCommsTask.controllerQueue->sendPayload(PRINT_THIS);

      if (ctrlrCommsTask.printRxFromController)
        ControllerData::print(ctrlrCommsTask.controllerQueue->payload, "-------------------------------------------\n[controllerPacketAvailable_cb]-->");

      vTaskDelay(TICKS_5ms);
    }
  }
}