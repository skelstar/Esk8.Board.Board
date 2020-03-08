#include <vesc_comms.h>

#define MOTOR_POLE_PAIRS 7
#define WHEEL_DIAMETER_MM 97
#define MOTOR_PULLEY_TEETH 15
#define WHEEL_PULLEY_TEETH 36 // https://hobbyking.com/en_us/gear-set-with-belt.html
#define NUM_BATT_CELLS 11

#define VESC_UART_BAUDRATE 115200

#define POWERING_DOWN_BATT_VOLTS_START NUM_BATT_CELLS * 3.0

vesc_comms vesc;

uint8_t vesc_packet[PACKET_MAX_LENGTH];

void try_get_values_from_vesc();

#include <vesc_utils.h>
//-----------------------------------------------------------------------

elapsedMillis since_got_values_from_vesc = 0;

//-----------------------------------------------------------------------------------
void send_to_vesc(uint8_t throttle, bool cruise_control)
{
  if (cruise_control)
  {
    vesc.setNunchuckValues(127, throttle, cruise_control, 0);
  }
  else
  {
    vesc.setNunchuckValues(127, throttle, 0, 0);
  }
}
//-----------------------------------------------------------------------------------
void vesc_update()
{
  // get values from vesc
  if (since_got_values_from_vesc > GET_FROM_VESC_INTERVAL)
  {
    since_got_values_from_vesc = 0;

#ifdef SEND_TO_VESC
    try_get_values_from_vesc();
#endif
  }
}
//-----------------------------------------------------------------------
void try_get_values_from_vesc()
{
  bool success = get_vesc_values();
  if (success)
  {
    if (vesc_powering_down())
    {
    }
    else if (board_packet.moving)
    {
      send_to_event_queue(xEV_MOVING);
    }
    else if (board_packet.moving == false)
    {
      send_to_event_queue(xEV_STOPPED);
    }
  }
}
//-----------------------------------------------------------------------------------
