
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
  EV_NRF_FIRST_PACKET,
};

// prototypes
void PRINT_NRF_STATE(const char *state_name);
void NRF_EVENT(NrfEvent x, char *s);

elapsedMillis since_requested;

#define SEND_TO_BOARD_INTERVAL 1000

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
      else if (controller_packet.id == 0)
      {
        NRF_EVENT(EV_NRF_FIRST_PACKET, "EV_NRF_FIRST_PACKET");
      }
      // else if (since_last_controller_packet > controller_config.send_interval + 100)
      else if (since_last_controller_packet > SEND_TO_BOARD_INTERVAL + 100)
      {
        DEBUGVAL(SEND_TO_BOARD_INTERVAL, since_last_controller_packet);
        NRF_EVENT(EV_NRF_TIMING_OUT, "EV_NRF_TIMING_OUT (nrf_normal)");
      }
    }, 
    NULL);
//-------------------------------------------------------
State nrf_requested(
    [] {
      PRINT_NRF_STATE("NRF: nrf_requested.........");
      controller_packet.command = 0;
    },
    [] {
      if (send_packet_to_controller(ReasonType::REQUESTED))
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
      if (send_packet_to_controller(ReasonType::REQUESTED) == false)
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
State nrf_got_first_packet(
    [] {
      PRINT_NRF_STATE("NRF: nrf_got_first_packet.........");
    },
    [] {
    },
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
  // nrf_timing_out
  nrf_fsm.add_transition(&nrf_timing_out, &nrf_normal, EV_NRF_RESPONDED, NULL);
  nrf_fsm.add_transition(&nrf_timing_out, &nrf_timedout, EV_NRF_TIMED_OUT, NULL);
  // nrf_timed_out
  nrf_fsm.add_transition(&nrf_timedout, &nrf_normal, EV_NRF_PACKET, NULL);
  // first packet
  nrf_fsm.add_transition(&nrf_requested, &nrf_got_first_packet, EV_NRF_FIRST_PACKET, NULL);
  nrf_fsm.add_transition(&nrf_got_first_packet, &nrf_normal, EV_NRF_PACKET, NULL);
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
    Serial.printf("--> NRF EVENT: %s \n", s);
#endif
  }
  nrf_fsm.trigger(x);
  nrf_fsm.run_machine();
}
