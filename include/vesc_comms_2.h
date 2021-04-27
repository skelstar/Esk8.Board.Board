#include <vesc_comms.h>

#define MOTOR_POLE_PAIRS 7
#define WHEEL_DIAMETER_MM 97
#define MOTOR_PULLEY_TEETH 15
#define WHEEL_PULLEY_TEETH 36 // https://hobbyking.com/en_us/gear-set-with-belt.html
#define NUM_BATT_CELLS 11

#define POWERING_DOWN_BATT_VOLTS_START NUM_BATT_CELLS * 3.0

void try_get_values_from_vesc();
VescData *get_vesc_values();

vesc_comms vesc;

#include <vesc_utils.h>
//-----------------------------------------------------------------------

elapsedMillis since_got_values_from_vesc = 0;

//-----------------------------------------------------------------------------------
void send_to_vesc(uint8_t throttle, bool cruise_control)
{
  vesc.setNunchuckValues(127, throttle, cruise_control, 0);
}
//-----------------------------------------------------------------------------------
void vesc_update()
{
  // get values from vesc
  if (since_got_values_from_vesc > GET_FROM_VESC_INTERVAL)
  {
    since_got_values_from_vesc = 0;

    if (SEND_TO_VESC)
      try_get_values_from_vesc();
  }
}
//-----------------------------------------------------------------------
void try_get_values_from_vesc()
{
  using namespace Comms;
  VescData *board_packet1 = get_vesc_values();

  if (board_packet1 != nullptr)
  {
    if (vescQueue != nullptr)
      vescQueue->send(board_packet1);

    commsFsm.trigger(EV_VESC_SUCCESS);
  }
  else
  {
    commsFsm.trigger(EV_VESC_FAILED);
  }
}
//-----------------------------------------------------------------------------------
VescData *get_vesc_values()
{
  uint8_t vesc_packet[PACKET_MAX_LENGTH];
  vesc_comms vesc;
  VescData *res;

  bool success = vesc.fetch_packet(vesc_packet) > 0;

  if (!success)
    return nullptr;

  rpm_raw = vesc.get_rpm(vesc_packet);

  res->batteryVoltage = vesc.get_voltage(vesc_packet);
  res->moving = rpm_raw > RPM_AT_MOVING;

  res->ampHours = vesc.get_amphours_discharged(vesc_packet) - initial_ampHours;
  res->motorCurrent = vesc.get_motor_current(vesc_packet);
  res->odometer = get_distance_in_meters(vesc.get_tachometer(vesc_packet)) - initial_odometer;
  res->temp_mosfet = vesc.get_temp_mosfet(vesc_packet);

  return res;
}
//-----------------------------------------------------------------------------------
