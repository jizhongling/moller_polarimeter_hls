#ifndef hls_moller_helicity_h
#define hls_moller_helicity_h

#include <hls_stream.h>
#define AP_INT_MAX_W 4096 // Must be defined before next line
#include <ap_int.h>

typedef ap_uint<4096> vxs_payload_t;
typedef ap_uint<128>  vetroc_t;
typedef struct
{
  ap_uint<13> e;
  ap_uint<3>  t;
} fadc_t;

#define N_TRIGGER_BITS  5

// trig_t:
//   - n[N] N trigger bits
//   - n[]: 8bits every 32ns for trigger decision (at 4ns resolution)
//   - bit definition: 0 = trigger false, 1 = trigger true
//   - e.g. trigger bit 0: trig_t.n[0][0] @ 0ns, trig_t.n[0][1] @ 4ns, ..., trig_t.n[0][7] @ 24ns
//          trigger bit 1: trig_t.n[1][0] @ 0ns, trig_t.n[1][1] @ 4ns, ..., trig_t.n[1][7] @ 24ns
typedef struct
{
  ap_uint<1> n[N_TRIGGER_BITS][8];
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
vetroc_t get_vetroc(vxs_payload_t vxs_payload, int slot, ap_uint<3> t);
fadc_t get_fadc(vxs_payload_t vxs_payload, int slot, int ch);

// Top-level synthesis target
void hls_moller_compton(
    // FADC & VETROC Streaming (input)
    hls::stream<vxs_payload_t>  &s_vxs_payload,

    // Compton trigger parameters & trigger bit stream (output)
    ap_uint<13>                 fadc_threshold[5],
    ap_uint<3>                  mult_threshold[5],
    ap_uint<4>                  e_plane_mask[5],
    ap_uint<16>                 fadc_mask[5],
    hls::stream<trig_t>         &s_trig,

    // Helicity event builder
    hls::stream<eventdata_t>    &s_event_helicity,
    hls::stream<trig_accept_t>  &s_trig_accept,
    ap_uint<32>                 &helicity_cnt,
    ap_uint<32>                 &mps_cnt,

    // Helicity scalers event builder
    hls::stream<eventdata_t>    &s_event_scalers
  );

#endif
