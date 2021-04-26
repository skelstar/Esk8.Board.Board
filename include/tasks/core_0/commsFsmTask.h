

namespace Comms
{
  // const char *name = "COMMS";
  // bool taskReady = false;

  // void printFsmState_cb(uint16_t id)
  // {
  //   if (PRINT_COMMS_FSM_STATE)
  //     Serial.printf(PRINT_STATE_FORMAT, name, getStateName(StateID(id)));
  // }

  // void printFsmTrigger_cb(uint16_t ev)
  // {
  //   if (PRINT_COMMS_FSM_TRIGGER)
  //     Serial.printf(PRINT_STATE_FORMAT, name, getEvent(Event(ev)));
  // }

  // //----------------------------------------------
  // Event lastCommsEvent = EV_NONE;

  // State stOffline(
  //     [] {
  //       commsFsm.printState(OFFLINE);
  //       controller_connected = false;
  //     },
  //     NULL, NULL);

  // State stCtrlrOnline(
  //     [] {
  //       commsFsm.printState(CTRLR_ONLINE);
  //       controller_connected = true;
  //     },
  //     NULL, NULL);

  // State stVescOnline(
  //     [] {
  //       commsFsm.printState(VESC_ONLINE);
  //     },
  //     NULL, NULL);

  // State stCtrlrVescOnline(
  //     [] {
  //       commsFsm.printState(CTRLR_VESC_ONLINE);
  //     },
  //     NULL, NULL);

  // Fsm fsm(&stOffline);

  // //------------------------------------------------------
  // void addTransitions()
  // {
  //   // state_offline
  //   fsm.add_transition(&stOffline, &stCtrlrOnline, EV_CTRLR_PKT, NULL);
  //   fsm.add_transition(&stOffline, &stVescOnline, EV_VESC_SUCCESS, NULL);

  //   // stCtrlrOnline
  //   fsm.add_transition(&stCtrlrOnline, &stOffline, EV_CTRLR_TIMEOUT, NULL);
  //   fsm.add_transition(&stCtrlrOnline, &stCtrlrVescOnline, EV_VESC_SUCCESS, NULL);

  //   // stVescOnline
  //   fsm.add_transition(&stVescOnline, &stCtrlrVescOnline, EV_CTRLR_PKT, NULL);
  //   fsm.add_transition(&stVescOnline, &stOffline, EV_VESC_FAILED, NULL);

  //   // stCtrlrVescOnline
  //   fsm.add_transition(&stCtrlrVescOnline, &stVescOnline, EV_CTRLR_TIMEOUT, NULL);
  //   fsm.add_transition(&stCtrlrVescOnline, &stCtrlrOnline, EV_VESC_FAILED, NULL);
  // }

  // //========================================================

  // void task(void *pvParameters)
  // {
  //   Serial.printf("CommsFsm Task running on CORE_%d\n", xPortGetCoreID());

  //   using namespace Comms;
  //   commsFsm.begin(&Comms::fsm);
  //   commsFsm.setPrintStateCallback(printFsmState_cb);
  //   commsFsm.setPrintTriggerCallback(printFsmTrigger_cb);

  //   addTransitions();

  //   taskReady = true;

  //   while (true)
  //   {
  //     Comms::commsFsm.runMachine();

  //     vTaskDelay(10);
  //   }
  //   vTaskDelete(NULL);
  // }

  // void createTask(uint8_t core, uint8_t priority)
  // {
  //   xTaskCreatePinnedToCore(
  //       task,
  //       "CommsFsm Task",
  //       5000,
  //       NULL,
  //       priority,
  //       NULL,
  //       core);
  // }
} // namespace Comms