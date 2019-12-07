#include <vesc_comms.h>

#define MOTOR_POLE_PAIRS 7
#define WHEEL_DIAMETER_MM 97
#define MOTOR_PULLEY_TEETH 15
#define WHEEL_PULLEY_TEETH 36 // https://hobbyking.com/en_us/gear-set-with-belt.html
#define NUM_BATT_CELLS 11

uint8_t vesc_packet[PACKET_MAX_LENGTH];

VescData vescdata, initialVescData;
ControllerData controller_packet, old_packet;

#define VESC_UART_BAUDRATE 115200

#define POWERING_DOWN_BATT_VOLTS_START NUM_BATT_CELLS * 3.0

vesc_comms vesc;

//-----------------------------------------------------------------------------------

int32_t rotations_to_meters(int32_t rotations);
float getDistanceInMeters(int32_t tacho);

//-----------------------------------------------------------------------------------
void send_to_vesc(uint8_t throttle)
{
  vesc.setNunchuckValues(127, throttle, 0, 0);
  DEBUGVAL("Sent to motor", throttle);
}

//-----------------------------------------------------------------------------------
bool getVescValues()
{
  bool success = vesc.fetch_packet(vesc_packet) > 0;

  if (success)
  {
    vescdata.batteryVoltage = vesc.get_voltage(vesc_packet);
    vescdata.moving = vesc.get_rpm(vesc_packet) > 50;
    vescdata.ampHours = vesc.get_amphours_discharged(vesc_packet);
    vescdata.odometer = getDistanceInMeters(/*tacho*/ vesc.get_tachometer(vesc_packet));
  }
  else {
    vescdata.batteryVoltage = 0.0;
    vescdata.moving = false;
    vescdata.ampHours = 0.0;
    vescdata.odometer = 0.0;
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