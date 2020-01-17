


#ifndef Arduino
#include <Arduino.h>
#endif
#ifndef VescData
#include <VescData.h>
#endif

void try_get_values_from_vesc();

elapsedMillis since_got_values_from_vesc = 0;

#define GET_FROM_VESC_INTERVAL 1000

void vescTask_0(void *pvParameters)
{
  Serial.printf("\nvescTask_0 running on core %d\n", xPortGetCoreID());

#ifndef SEND_TO_VESC
  Serial.printf("*** NOT SENDING TO VESC\n");
#endif

  vesc.init(VESC_UART_BAUDRATE);

  while (true)
  {
    // get values from vesc
    if (since_got_values_from_vesc > GET_FROM_VESC_INTERVAL)
    {
      since_got_values_from_vesc = 0;
      try_get_values_from_vesc();
    }

    // send to vesc
    uint8_t e;
    bool send_to_vesc_now = xQueueReceive(xSendToVescQueue, &e, pdMS_TO_TICKS(0)) == pdPASS;
    if (send_to_vesc_now)
    {
#ifdef SEND_TO_VESC
      send_to_vesc(controller_packet.throttle);
#endif
    }

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}




#ifdef FAKE_VESC_ONLINE
bool fake_vesc_online = true;
#else
bool fake_vesc_online = false;
#endif

void try_get_values_from_vesc()
{
  if (xVescDataSemaphore != NULL && xSemaphoreTake(xVescDataSemaphore, (TickType_t)10) == pdTRUE)
  {
    vescOnline = getVescValues() == true;
    xSemaphoreGive(xVescDataSemaphore);
  }

  if (vescOnline == false && !fake_vesc_online)
  {
    // send_to_fsm_event_queue(EV_VESC_OFFLINE);
  }
  else
  {
    if (vescPoweringDown())
    {
      // send_to_fsm_event_queue(EV_POWERING_DOWN);
    }
    else if (vescdata.moving)
    {
      // send_to_fsm_event_queue(EV_MOVING);
    }
    else
    {
      // send_to_fsm_event_queue(EV_STOPPED);
    }
  }
}
