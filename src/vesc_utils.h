#include <vesc_comms.h>

#define MOTOR_POLE_PAIRS 7
#define WHEEL_DIAMETER_MM 97
#define MOTOR_PULLEY_TEETH 15
#define WHEEL_PULLEY_TEETH 36 // https://hobbyking.com/en_us/gear-set-with-belt.html
#define NUM_BATT_CELLS 11

uint8_t vesc_packet[PACKET_MAX_LENGTH];

struct VESC_DATA
{
  float batteryVoltage;
  //   float motorCurrent;
  bool moving;
  float ampHours;
  float odometer;
} vescdata, dummyData;

#define VESC_UART_BAUDRATE 115200

#define POWERING_DOWN_BATT_VOLTS_START NUM_BATT_CELLS * 3.0

vesc_comms vesc;

//-----------------------------------------------------------------------------------

int32_t rotations_to_meters(int32_t rotations);
float getDistanceInMeters(int32_t tacho);

//-----------------------------------------------------------------------------------
bool getVescValues()
{
  bool success = vesc.fetch_packet(vesc_packet) > 0;

  if (success)
  {
    vescdata.batteryVoltage = vesc.get_voltage(vesc_packet);
    vescdata.moving = vesc.get_rpm(vesc_packet) > 50;
    // vescdata.motorCurrent = vesc.get_motor_current(vesc_packet);
    vescdata.ampHours = vesc.get_amphours_discharged(vesc_packet);
    vescdata.odometer = getDistanceInMeters(/*tacho*/ vesc.get_tachometer(vesc_packet));
  }
  else
  {
    dummyData.batteryVoltage += 0.1;
    dummyData.moving = false;
    dummyData.ampHours += 0.1;
    dummyData.odometer += 0.1;
  }
  return success;
}

bool vescPoweringDown()
{
  return vescdata.batteryVoltage < POWERING_DOWN_BATT_VOLTS_START && vescdata.batteryVoltage > 10;
}

//-----------------------------------------------------------------------------------
int32_t rotations_to_meters(int32_t rotations)
{
  float gear_ratio = float(WHEEL_PULLEY_TEETH) / float(MOTOR_PULLEY_TEETH);
  return (rotations / MOTOR_POLE_PAIRS / gear_ratio) * WHEEL_DIAMETER_MM * PI / 1000;
}
//-----------------------------------------------------------------------------------
float getDistanceInMeters(int32_t tacho)
{
  return rotations_to_meters(tacho / 6) / 1000.0;
}
//-----------------------------------------------------------------------------------
void waitForFirstPacketFromVesc()
{
  while (getVescValues() == false)
  {
    delay(1);
    yield();
  }
  // just got first packet
}