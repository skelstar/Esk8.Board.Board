

namespace Comms
{
  const char *name = "COMMS";
  bool taskReady = false;

  void printFsmState_cb(uint16_t id)
  {
    if (PRINT_COMMS_FSM_STATE)
      Serial.printf(PRINT_STATE_FORMAT, name, getStateName(StateID(id)));
  }

  void printFsmTrigger_cb(uint16_t ev)
  {
    if (PRINT_COMMS_FSM_TRIGGER)
      Serial.printf(PRINT_STATE_FORMAT, name, getEvent(Event(ev)));
  }

  //----------------------------------------------

  void task(void *pvParameters)
  {
    Serial.printf("CommsFsm Task running on CORE_%d\n", xPortGetCoreID());

    using namespace Comms;
    commsFsm.begin(&Comms::fsm);
    commsFsm.setPrintStateCallback(printFsmState_cb);
    commsFsm.setPrintTriggerCallback(printFsmTrigger_cb);

    addTransitions();

    taskReady = true;

    while (true)
    {
      Comms::commsFsm.runMachine();

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }

  void createTask(uint8_t core, uint8_t priority)
  {
    xTaskCreatePinnedToCore(
        task,
        "CommsFsm Task",
        5000,
        NULL,
        priority,
        NULL,
        core);
  }
} // namespace Comms