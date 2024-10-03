#ifndef hls_moller_ecal_h
#define hls_moller_ecal_h

#include <hls_stream.h>
#define AP_INT_MAX_W 4096 // Must be defined before next line
#include <ap_int.h>

const int slot = 3;
const int nch = 8;
const int nt = 8;
const int dt = 3;

typedef ap_uint<4096> vxs_payload_t;
typedef struct
{
  ap_uint<13> e;
  ap_uint<3>  t;
} fadc_t;

#define N_TRIGGER_BITS 1

// trig_t:
//   - n[N] N trigger bits
//   - n[]: 8bits every 32ns for trigger decision (at 4ns resolution)
//   - bit definition: 0 = trigger false, 1 = trigger true
//   - e.g. trigger bit 0: trig_t.n[0][0] @ 0ns, trig_t.n[0][1] @ 4ns, ..., trig_t.n[0][7] @ 24ns
//          trigger bit 1: trig_t.n[1][0] @ 0ns, trig_t.n[1][1] @ 4ns, ..., trig_t.n[1][7] @ 24ns
typedef struct
{
  ap_uint<1> n[N_TRIGGER_BITS][nt];
} trig_t;

typedef struct
{
  ap_uint<10> t_start;
  ap_uint<10> t_stop;
} trig_accept_t;

/* readout data */
typedef struct
{
  ap_uint<32> data;
  ap_uint<1>  end;
} eventdata_t;

// Helper functions
fadc_t get_fadc(vxs_payload_t vxs_payload, int slot, int ch);

// Top-level synthesis target
void hls_moller_ecal(
    // FADC Streaming (input)
    hls::stream<vxs_payload_t>  &s_vxs_payload,

    // ECal trigger parameters & trigger bit stream (output)
    hls::stream<trig_t>         &s_trig,
    vxs_payload_t 				&vxs_payload_last,
    ap_uint<13>                 &fadc_threshold,

    // Helicity event builder
    hls::stream<eventdata_t>    &s_event_helicity,
    hls::stream<trig_accept_t>  &s_trig_accept,
    ap_uint<32>                 &helicity_cnt,
    ap_uint<32>                 &mps_cnt,

    // Helicity scalers event builder
    hls::stream<eventdata_t>    &s_event_scalers
    );

#endif
