#ifndef hls_ecal_h
#define hls_ecal_h

#include <hls_stream.h>
#define AP_INT_MAX_W 4096 // Must be defined before next line
#include <ap_int.h>
#include "hls_moller_ecal.h"

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
void hls_ecal(
    hls::stream<vxs_payload_t> &s_vxs_payload,
    hls::stream<trig_t> &s_trig,
    vxs_payload_t &vxs_payload_last,
    ap_uint<13> fadc_threshold
    );

#endif
