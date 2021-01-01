#ifndef Arduino
#include <Arduino.h>
#endif

//------------------------------------------------------

bool boardIs(String chipId, String compareId)
{
  return chipId == compareId;
}
//------------------------------------------------------

void print_build_status(String chipId)
{
  Serial.printf("\n");
  Serial.printf("-----------------------------------------------\n");
  Serial.printf("               Esk8.Board.Board \n");
  Serial.printf("               Chip id: %s\n", chipId.c_str());

  if (chipId == M5STACKFIREID)
  {
    Serial.printf("               M5STACK-FIRE\n");
  }
  else
  {
    Serial.printf("               Board: unknown\n");
  }

#ifdef RELEASE_BUILD
  Serial.printf("-----------------------------------------------\n");
  Serial.printf("               RELEASE BUILD!! \n");
  Serial.printf("-----------------------------------------------\n");
  Serial.printf("               %s \n", __TIME__);
  Serial.printf("               %s \n", __DATE__);
  Serial.printf("-----------------------------------------------\n");
#endif

#ifdef DEBUG_BUILD
  Serial.printf("-----------------------------------------------\n");
  Serial.printf("               DEBUG BUILD!! \n");
  Serial.printf("-----------------------------------------------\n");
  Serial.printf("               %s \n", __TIME__);
  Serial.printf("               %s \n", __DATE__);
  Serial.printf("-----------------------------------------------\n");
#endif
  // Serial.printf("\n");
}
//------------------------------------------------------

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

#define BATTERY_VOLTAGE_FULL 4.2 * 11         // 46.2
#define BATTERY_VOLTAGE_CUTOFF_START 3.4 * 11 // 37.4
#define BATTERY_VOLTAGE_CUTOFF_END 3.1 * 11   // 34.1
//------------------------------------------------------

uint8_t getBatteryPercentage(float voltage)
{
  float voltsLeft = voltage - BATTERY_VOLTAGE_CUTOFF_END;
  float voltsAvail = BATTERY_VOLTAGE_FULL - BATTERY_VOLTAGE_CUTOFF_END;

  uint8_t percent = 0;
  if (voltage > BATTERY_VOLTAGE_CUTOFF_END)
  {
    percent = (voltsLeft / voltsAvail) * 100;
  }
  if (percent > 100)
  {
    percent = 100;
  }
  return percent;
}
//------------------------------------------------------

char debugTime[20];

double getDebugTime()
{
  return millis() / 1000.0;
}
//------------------------------------------------------

char *getCDebugTime(const char *format = "%6.1fs")
{
  sprintf(debugTime, format, getDebugTime());
  return debugTime;
}
//------------------------------------------------------
