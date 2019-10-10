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