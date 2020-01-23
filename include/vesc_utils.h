
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
    board_packet.batteryVoltage = vesc.get_voltage(vesc_packet);
    board_packet.moving = vesc.get_rpm(vesc_packet) > 50;
    //board_packet.ampHours = vesc.get_amphours_discharged(vesc_packet);
    board_packet.odometer = get_distance_in_meters(vesc.get_tachometer(vesc_packet));
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