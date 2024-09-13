#ifndef hls_moller_ecal_h
#define hls_moller_ecal_h

#include <hls_stream.h>
#define AP_INT_MAX_W 4096 // Must be defined before next line
#include <ap_int.h>

const int slot = 3;
const int nch = 8;
const int nt = 8;
const int dt = 3;
const int emax = 100;
const int eth = 0;

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

// Helper functions
fadc_t get_fadc(vxs_payload_t vxs_payload, int slot, int ch);

// hls_moller_ecal:
//   - s_vxs_payload: contains 16 slots of VXS payload information updated every 32ns
//     for compton we assume the following crate configuration:
//       FADC   in slot 3  photon detector
//
//   - s_trig: contains trigger bit decisions, currently 1 trigger:
//       s_trig.n[0]
//
//   - Currently this code expects:
//       1) FADC hits
void hls_moller_ecal(
    hls::stream<vxs_payload_t> &s_vxs_payload,
    hls::stream<trig_t> &s_trig,
    vxs_payload_t &vxs_payload_last,
    ap_uint<13> fadc_threshold
    );

#endif
