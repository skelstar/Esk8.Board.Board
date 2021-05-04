#ifndef Arduino
#include <Arduino.h>
#endif

#ifndef SOFT_SPI_MOSI_PIN
#define SOFT_SPI_MOSI_PIN 0
#endif
#ifndef SOFT_SPI_MISO_PIN
#define SOFT_SPI_MISO_PIN 0
#endif
#ifndef SOFT_SPI_CLK_PIN
#define SOFT_SPI_CLK_PIN 0
#endif

//------------------------------------------------------

bool boardIs(String chipId, String compareId)
{
  return chipId == compareId;
}
//------------------------------------------------------

void print_build_status(String chipId)
{
  const char *line = "-----------------------------------------------\n";
  const char *spaces = "    ";

  Serial.printf("\n");
  Serial.printf(line);
  Serial.printf("%s Esk8.Board.Board \n", spaces);
  Serial.printf("%s Chip id: %s\n", chipId.c_str());

  if (chipId == M5STACKFIREID)
  {
    Serial.printf("%s M5STACK-FIRE\n", spaces);
    Serial.printf("%s MISO: %d\n", spaces, MISO);
    Serial.printf("%s MOSI: %d\n", spaces, MOSI);
    Serial.printf("%s SCK:  %d\n", spaces, SCK);
    Serial.printf("%s CS:   %d\n", spaces, SPI_CS);
    Serial.printf("%s CE:   %d\n", spaces, SPI_CE);
    Serial.printf("\n");
  }
  else if (chipId == TDISPLAYBOARD ||
           chipId == TDISPLAYBOARD_BROWN)
  {
    Serial.printf("%s T-DISPLAY BOARD\n", spaces);
    Serial.printf("%s MISO: %d\n", spaces, SOFT_SPI_MISO_PIN);
    Serial.printf("%s MOSI: %d\n", spaces, SOFT_SPI_MOSI_PIN);
    Serial.printf("%s SCK:  %d\n", spaces, SOFT_SPI_CLK_PIN);
    Serial.printf("%s CS:   %d\n", spaces, SPI_CS);
    Serial.printf("%s CE:   %d\n", spaces, SPI_CE);
    Serial.printf("\n");
  }
  else
  {
    Serial.printf("%s Board: unknown\n", spaces);
    Serial.printf("%s MISO: %d\n", spaces, MISO);
    Serial.printf("%s MOSI: %d\n", spaces, MOSI);
    Serial.printf("%s SCK:  %d\n", spaces, SCK);
    Serial.printf("%s CS:   %d\n", spaces, SPI_CS);
    Serial.printf("%s CE:   %d\n", spaces, SPI_CE);
    Serial.printf("\n");
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
