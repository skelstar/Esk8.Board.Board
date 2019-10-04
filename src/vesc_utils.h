#include <vesc_comms.h>

#define MOTOR_POLE_PAIRS 7
#define WHEEL_DIAMETER_MM 97
#define MOTOR_PULLEY_TEETH 15
#define WHEEL_PULLEY_TEETH 36 // https://hobbyking.com/en_us/gear-set-with-belt.html
#define NUM_BATT_CELLS 11

uint8_t vesc_packet[PACKET_MAX_LENGTH];

#define GET_FROM_VESC_INTERVAL 1000

struct VESC_DATA
{
  float batteryVoltage;
  //   float motorCurrent;
  bool moving;
  //   float ampHours;
  float odometer;
};
VESC_DATA vescdata;

#define VESC_UART_BAUDRATE 115200

#define POWERING_DOWN_BATT_VOLTS_START NUM_BATT_CELLS * 3.0

vesc_comms vesc;

//-----------------------------------------------------------------------------------

enum vesc_eventsEnum
{
  STARTUP,
  ONLINE,
  OFFLINE,
  MOVING,
  STOPPED
};

vesc_eventsEnum current_state = OFFLINE;

State state_board_startup([] {
  current_state = STARTUP;
  Serial.printf("state: STARTUP\n");
  initialiseApp();
},
NULL,
NULL);

State state_board_offline([] {
  current_state = OFFLINE;
  Serial.printf("state: OFFLINE [INIT]\n");
  displayPopup("offline");
},
                          NULL, NULL);

State state_board_online([] {
  current_state = ONLINE;
  Serial.printf("state: ONLINE\n");
  drawBattery(65);
},
                         NULL, NULL);

State state_board_moving([] {
  current_state = MOVING;
  Serial.printf("state: MOVING\n");
},
                         NULL, NULL);

State state_board_stopped([] {
  current_state = STOPPED;
  Serial.printf("state: STOPPED\n");
},
                          NULL, NULL);

//--
// https://github.com/jonblack/arduino-fsm/blob/master/examples/light_switch/light_switch.ino

void addVescFsmTransitions(Fsm *fsm)
{
  uint8_t vesc_event = STARTUP;
  
  vesc_event = STARTUP;
  fsm->add_transition(&state_board_startup, &state_board_startup, vesc_event, NULL);
  
  vesc_event = ONLINE;
  fsm->add_transition(&state_board_offline, &state_board_online, vesc_event, NULL);

  vesc_event = OFFLINE;
  fsm->add_transition(&state_board_offline, &state_board_offline, vesc_event, NULL);
  fsm->add_transition(&state_board_online, &state_board_offline, vesc_event, NULL);
  fsm->add_transition(&state_board_moving, &state_board_offline, vesc_event, NULL);
  fsm->add_transition(&state_board_stopped, &state_board_offline, vesc_event, NULL);

  vesc_event = MOVING;
  fsm->add_transition(&state_board_offline, &state_board_moving, vesc_event, NULL);
  fsm->add_transition(&state_board_online, &state_board_moving, vesc_event, NULL);
  fsm->add_transition(&state_board_stopped, &state_board_moving, vesc_event, NULL);

  vesc_event = STOPPED;
  fsm->add_transition(&state_board_online, &state_board_stopped, vesc_event, NULL);
  fsm->add_transition(&state_board_moving, &state_board_stopped, vesc_event, NULL);
}

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
    // vescdata.ampHours = vesc.get_amphours_discharged(vesc_packet);
    vescdata.odometer = getDistanceInMeters(/*tacho*/ vesc.get_tachometer(vesc_packet));
  }
  else
  {
    vescdata.batteryVoltage = 0.0;
    vescdata.moving = false;
    // vescdata.motorCurrent = 0.0;
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