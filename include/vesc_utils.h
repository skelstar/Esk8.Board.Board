
int32_t rpm_raw;
float initial_ampHours = 0.0;
float initial_odometer = 0.0;

//-----------------------------------------------------------------------------------
int32_t rotations_to_meters(int32_t rotations)
{
  float gear_ratio = float(WHEEL_PULLEY_TEETH) / float(MOTOR_PULLEY_TEETH);
  return (rotations / MOTOR_POLE_PAIRS / gear_ratio) * WHEEL_DIAMETER_MM * PI / 1000;
}
//-----------------------------------------------------------------------------------
float get_distance_in_meters(int32_t tacho)
{
  return rotations_to_meters(tacho / 6) / 1000.0;
}
//-----------------------------------------------------------------------------------
bool get_vesc_values()
{
  bool success = vesc.fetch_packet(vesc_packet) > 0;

  if (success)
  {
    rpm_raw = vesc.get_rpm(vesc_packet);
    if (board_packet.id == 0)
    {
      // allow for any residual measurements/data
      initial_ampHours = vesc.get_amphours_discharged(vesc_packet);
      initial_odometer = get_distance_in_meters(vesc.get_tachometer(vesc_packet));
    }

    board_packet.batteryVoltage = vesc.get_voltage(vesc_packet);
    board_packet.moving = rpm_raw > RPM_AT_MOVING;

    board_packet.ampHours = vesc.get_amphours_discharged(vesc_packet) - initial_ampHours;
    board_packet.motorCurrent = vesc.get_motor_current(vesc_packet);
    board_packet.odometer = get_distance_in_meters(vesc.get_tachometer(vesc_packet)) - initial_odometer;
  }
  else
  {
    board_packet.moving = false;
  }
  return success;
}
//-----------------------------------------------------------------------------------
bool vesc_powering_down()
{
  return false; //vescdata.batteryVoltage < POWERING_DOWN_BATT_VOLTS_START && vescdata.batteryVoltage > 10;
}
//-----------------------------------------------------------------------------------