#ifndef Arduino
#include <Arduino.h>
#endif

void storageReport(float actualAmphours, float actualOdometer, VescData initialVescData, float amphoursTotal, float odometerTotal)
{
  Serial.printf("Powering down. Storing:\n- ampHours = %.1fmAh (%.1fmAh)\n- odometer = %.1fkm (%.1fkm)\n",
                actualAmphours,
                initialVescData.ampHours,
                actualOdometer,
                initialVescData.odometer);
  Serial.printf("Total stats:\n- %.1fmAh (%.1fmAh + %.1fmAh)\n- %.1fkm (%.1fkm + %.1fkm)\n",
                amphoursTotal + actualAmphours,
                amphoursTotal,
                actualAmphours,
                odometerTotal + actualOdometer,
                odometerTotal,
                actualOdometer);
}

char* reason_toString(ReasonType reason)
{
  switch (reason)
  {
    case BOARD_STOPPED:
      return "BOARD_STOPPED";
    case BOARD_MOVING:
      return "BOARD_MOVING";
    case FIRST_PACKET:
      return "FIRST_PACKET";
    case LAST_WILL:
      return "LAST_WILL";
    case REQUESTED:
      return "REQUESTED";
    default:
      return "unhandle reason";
  }
}
