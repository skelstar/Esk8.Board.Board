

#ifndef PRINT_THROTTLE
#define PRINT_THROTTLE 0
#endif

#define LCD_WIDTH 320
#define LCD_HEIGHT 240

#define BTN_A_XPOS 65
#define BTN_B_XPOS LCD_WIDTH / 2
#define BTN_C_XPOS LCD_WIDTH - BTN_A_XPOS

#define FOOTLIGHT_NUM_PIXELS 8

#define VESC_UART_BAUDRATE 115200

namespace FootLight
{
  enum Event
  {
    NO_EVENT = 0,
    MOVING,
    STOPPED,
  };

  const char *getEvent(uint16_t ev)
  {
    switch (ev)
    {
    case NO_EVENT:
      return "NO_EVENT";
    case MOVING:
      return "MOVING";
    case STOPPED:
      return "STOPPED";
    }
    return outOfRange("FootLight::getEvent()");
  }
} // namespace FootLight

//---------------------------------------------

namespace Comms
{
  enum Event
  {
    EV_NONE,
    EV_VESC_SUCCESS,
    EV_VESC_FAILED,
    EV_CTRLR_PKT,
    EV_CTRLR_TIMEOUT,
  };

  const char *getEvent(uint16_t ev)
  {
    switch (ev)
    {
    case EV_NONE:
      return "EV_NONE";
    case EV_VESC_SUCCESS:
      return "EV_VESC_SUCCESS";
    case EV_VESC_FAILED:
      return "EV_VESC_FAILED";
    case EV_CTRLR_PKT:
      return "EV_CTRLR_PKT";
    case EV_CTRLR_TIMEOUT:
      return "EV_CTRLR_TIMEOUT";
    }
    return outOfRange("Comms::getEvent()");
  }

  FsmManager<Comms::Event> commsFsm;

  enum StateID
  {
    OFFLINE = 0,
    CTRLR_ONLINE,
    VESC_ONLINE,
    CTRLR_VESC_ONLINE,
  };

  const char *getStateName(StateID id)
  {
    switch (id)
    {
    case OFFLINE:
      return " OFFLINE";
    case CTRLR_ONLINE:
      return " CTRLR_ONLINE";
    case VESC_ONLINE:
      return " VESC_ONLINE";
    case CTRLR_VESC_ONLINE:
      return " CTRLR_VESC_ONLINE";
    }
    return outOfRange("getStateName");
  }
} // namespace Comms

//---------------------------------------------

namespace M5StackDisplay
{
  enum StateID
  {
    ST_READY = 0,
    ST_MOVING,
    ST_REAL_MOVING,
    ST_STOPPED,
    ST_STOPPING,
  };
  const char *stateID(uint16_t id)
  {
    switch (id)
    {
    case ST_READY:
      return "READY";
    case ST_MOVING:
      return "MOVING";
    case ST_REAL_MOVING:
      return "ST_REAL_MOVING";
    case ST_STOPPED:
      return "ST_STOPPED";
    case ST_STOPPING:
      return "ST_STOPPING";
    }
    return outOfRange("FSM::stateID()");
  }

  enum Trigger
  {
    TR_NO_EVENT = 0,
    TR_MOVING,
    TR_STOPPED,
    TR_STOPPING,
  };

  const char *trigger(uint16_t ev)
  {
    switch (ev)
    {
    case TR_MOVING:
      return "MOVING";
    case TR_STOPPED:
      return "STOPPED";
    case TR_STOPPING:
      return "TR_STOPPING";
    }
    return outOfRange("FSM::event()");
  }

  enum QueueEvent
  {
    Q_NO_EVENT = 0,
    Q_MOVING,
    Q_RL_MOVING,
    Q_STOPPED,
    Q_RL_STOPPING,
  };

  const char *queueEvent(uint16_t ev)
  {
    switch (ev)
    {
    case Q_NO_EVENT:
      return "NO_EVENT";
    case Q_MOVING:
      return " MOVING ";
    case Q_RL_MOVING:
      return "Q_RL_MOVING";
    case Q_STOPPED:
      return "STOPPED";
    case Q_RL_STOPPING:
      return "Q_RL_STOPPING";
    }
    return outOfRange("M5StackDisplay name()");
  }
} // namespace M5StackDisplay

//---------------------------------------------

#ifndef SEND_TO_VESC
#define SEND_TO_VESC 0
#endif
#ifndef PRINT_THROTTLE
#define PRINT_THROTTLE 0
#endif
#ifndef PRINT_COMMS_FSM_STATE
#define PRINT_COMMS_FSM_STATE 0
#endif
#ifndef PRINT_COMMS_FSM_TRIGGER
#define PRINT_COMMS_FSM_TRIGGER 0
#endif
#ifndef PRINT_TX_TO_CONTROLLER
#define PRINT_TX_TO_CONTROLLER 0
#endif
#ifndef PRINT_RX_FROM_CONTROLLER
#define PRINT_RX_FROM_CONTROLLER 0
#endif
#ifndef PRINT_FOOTLIGHT_STATE_NAME
#define PRINT_FOOTLIGHT_STATE_NAME 0
#endif
#ifndef PRINT_LIGHT_FSM_EVENT_TRIGGER
#define PRINT_LIGHT_FSM_EVENT_TRIGGER 0
#endif
#ifndef PRINT_SEND_TO_FOOTLIGHT_QUEUE
#define PRINT_SEND_TO_FOOTLIGHT_QUEUE 0
#endif
#ifndef PRINT_READ_FROM_FOOTLIGHT_QUEUE
#define PRINT_READ_FROM_FOOTLIGHT_QUEUE 0
#endif
#ifndef PRINT_DISP_QUEUE_SEND
#define PRINT_DISP_QUEUE_SEND 0
#endif
#ifndef PRINT_DISP_QUEUE_READ
#define PRINT_DISP_QUEUE_READ 0
#endif
#ifndef PRINT_DISP_FSM_STATE
#define PRINT_DISP_FSM_STATE 0
#endif
#ifndef PRINT_DISP_FSM_TRIGGER
#define PRINT_DISP_FSM_TRIGGER 0
#endif
#ifndef USING_M5STACK
#define USING_M5STACK 0
#endif
#ifndef FEATURE_FOOTLIGHT
#define FEATURE_FOOTLIGHT 0
#endif
#ifndef MOCK_MOVING_WITH_BUTTON
#define MOCK_MOVING_WITH_BUTTON 0
#endif