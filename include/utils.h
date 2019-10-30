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

void fakeVescData()
{
  if (!debugPoweringDown)
  {
    vescdata.ampHours = vescdata.ampHours > 0.0 ? vescdata.ampHours + 0.23 : 12.0;
    vescdata.odometer = vescdata.odometer > 0.0 ? vescdata.odometer + 0.1 : 1.0;
    vescdata.batteryVoltage = 38.4;
    Serial.printf("DEBUG: ampHours %.1fmAh odo %.1fkm batt: %.1fv \n", vescdata.ampHours, vescdata.odometer, vescdata.batteryVoltage);
  }
  else
  {
    // powering down
    vescdata.batteryVoltage = vescdata.batteryVoltage > 0
                                  ? vescdata.batteryVoltage - 5
                                  : 0;
  }
}