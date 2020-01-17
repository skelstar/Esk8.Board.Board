


#ifndef Arduino
#include <Arduino.h>
#endif
#ifndef VescData
#include <VescData.h>
#endif


elapsedMillis since_got_values_from_vesc = 0;

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