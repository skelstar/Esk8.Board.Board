
#include <Fsm.h>

#ifndef VescData
#include <VescData.h>
#endif

enum NrfEvent
{
  EV_NRF_PACKET,
  EV_NRF_REQUESTED,
  EV_NRF_RESPONDED,
  EV_NRF_TIMING_OUT,
  EV_NRF_TIMED_OUT,
  EV_NRF_SEND_MOVING,
  EV_NRF_SEND_STOPPED,
};

// prototypes
void PRINT_NRF_STATE(const char *state_name);
void NRF_EVENT(NrfEvent x, char *s);

elapsedMillis since_requested;

//-------------------------------------------------------
State nrf_normal(
    [] {
      PRINT_NRF_STATE("NRF: nrf_normal.........");
    },
    [] {
      if (controller_packet.command == 1)
      {
        since_requested = 0;
        controller_packet.command = 0;
        NRF_EVENT(EV_NRF_REQUESTED, "EV_NRF_REQUESTED");
      }
      else if (since_last_controller_packet > CONTROLLER_TIMEOUT)
      {
        NRF_EVENT(EV_NRF_TIMING_OUT, "EV_NRF_TIMING_OUT");
      }
    },
    [] {
    });
//-------------------------------------------------------
State nrf_requested(
    [] {
      PRINT_NRF_STATE("NRF: nrf_requested.........");
      controller_packet.command = 0;
    },
    [] {
      if (send_to_packet_controller(ReasonType::REQUESTED))
      {
        NRF_EVENT(EV_NRF_RESPONDED, "EV_NRF_RESPONDED");
      }
      else
      {
        NRF_EVENT(EV_NRF_TIMING_OUT, "EV_NRF_TIMING_OUT");
      }
    },
    NULL);
//-------------------------------------------------------
State nrf_stopped(
    [] {
      PRINT_NRF_STATE("NRF: nrf_stopped.........");
    },
    [] {
      if (send_to_packet_controller(ReasonType::BOARD_STOPPED))
      {
        NRF_EVENT(EV_NRF_RESPONDED, "EV_NRF_RESPONDED");
      }
      else
      {
        NRF_EVENT(EV_NRF_TIMING_OUT, "EV_NRF_TIMING_OUT");
      }
    },
    NULL);
//-------------------------------------------------------
State nrf_moving(
    [] {
      PRINT_NRF_STATE("NRF: nrf_moving.........");
    },
    [] {
      if (send_to_packet_controller(ReasonType::BOARD_MOVING))
      {
        NRF_EVENT(EV_NRF_RESPONDED, "EV_NRF_RESPONDED");
      }
      else
      {
        NRF_EVENT(EV_NRF_TIMING_OUT, "EV_NRF_TIMING_OUT");
      }
    },
    NULL);
//-------------------------------------------------------
State nrf_timing_out(
    [] {
      PRINT_NRF_STATE("NRF: nrf_timing_out.........");
    },
    [] {
      if (send_to_packet_controller(ReasonType::REQUESTED) == false)
      {
        NRF_EVENT(EV_NRF_TIMED_OUT, "EV_NRF_TIMED_OUT");
      }
      else
      {
        NRF_EVENT(EV_NRF_RESPONDED, "EV_NRF_RESPONDED");
      }
    },
    NULL);
//-------------------------------------------------------
State nrf_timedout(
    [] {
      PRINT_NRF_STATE("NRF: nrf_timedout.........");
    },
    NULL,
    NULL);
//-------------------------------------------------------

Fsm nrf_fsm(&nrf_normal);

void add_nrf_fsm_transitions()
{
  // nrf_normal
  nrf_fsm.add_transition(&nrf_normal, &nrf_requested, EV_NRF_REQUESTED, NULL);
  nrf_fsm.add_transition(&nrf_normal, &nrf_timing_out, EV_NRF_TIMING_OUT, NULL);
  // nrf requested
  nrf_fsm.add_transition(&nrf_requested, &nrf_normal, EV_NRF_RESPONDED, NULL);
  nrf_fsm.add_transition(&nrf_requested, &nrf_timing_out, EV_NRF_TIMING_OUT, NULL);
  // nrf_stopped
  nrf_fsm.add_transition(&nrf_normal, &nrf_stopped, EV_NRF_SEND_STOPPED, NULL);
  nrf_fsm.add_transition(&nrf_requested, &nrf_stopped, EV_NRF_SEND_STOPPED, NULL);
  nrf_fsm.add_transition(&nrf_stopped, &nrf_timing_out, EV_NRF_TIMING_OUT, NULL);
  nrf_fsm.add_transition(&nrf_stopped, &nrf_normal, EV_NRF_RESPONDED, NULL);
  // nrf_moving
  nrf_fsm.add_transition(&nrf_normal, &nrf_moving, EV_NRF_SEND_MOVING, NULL);
  nrf_fsm.add_transition(&nrf_requested, &nrf_moving, EV_NRF_SEND_MOVING, NULL);
  nrf_fsm.add_transition(&nrf_moving, &nrf_timing_out, EV_NRF_TIMING_OUT, NULL);
  nrf_fsm.add_transition(&nrf_moving, &nrf_normal, EV_NRF_RESPONDED, NULL);
  // nrf_timing_out
  nrf_fsm.add_transition(&nrf_timing_out, &nrf_normal, EV_NRF_RESPONDED, NULL);
  nrf_fsm.add_transition(&nrf_timing_out, &nrf_timedout, EV_NRF_TIMED_OUT, NULL);
  // nrf_timed_out
  nrf_fsm.add_transition(&nrf_timedout, &nrf_normal, EV_NRF_PACKET, NULL);
}

//-------------------------------------------------------
void PRINT_NRF_STATE(const char *state_name)
{
#ifdef PRINT_NRF_FSM_STATE
  DEBUG(state_name);
#endif
}

void NRF_EVENT(NrfEvent x, char *s)
{
  if (s != NULL)
  {
#ifdef PRINT_NRF_FSM_EVENT
    Serial.printf("NRF EVENT: %s \n", s);
#endif
  }
  nrf_fsm.trigger(x);
  nrf_fsm.run_machine();
}
