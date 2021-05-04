
// int32_t rpm_raw;
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
bool vesc_powering_down()
{
  return false; //vescdata.batteryVoltage < POWERING_DOWN_BATT_VOLTS_START && vescdata.batteryVoltage > 10;
}
//-----------------------------------------------------------------------------------