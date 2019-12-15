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

#define BATTERY_VOLTAGE_FULL 4.2 * 11         // 46.2
#define BATTERY_VOLTAGE_CUTOFF_START 3.4 * 11 // 37.4
#define BATTERY_VOLTAGE_CUTOFF_END 3.1 * 11   // 34.1

uint8_t getBatteryPercentage(float voltage) {
  float voltsLeft = voltage - BATTERY_VOLTAGE_CUTOFF_END;
  float voltsAvail = BATTERY_VOLTAGE_FULL - BATTERY_VOLTAGE_CUTOFF_END;

  uint8_t percent = 0;
  if ( voltage > BATTERY_VOLTAGE_CUTOFF_END ) { 
    percent = (voltsLeft /  voltsAvail) * 100;
  }
  if (percent > 100) {
    percent = 100;
	}
  return percent;
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
    case VESC_OFFLINE:
      return "VESC_OFFLINE";
    default:
      return "unhandle reason";
  }
}
