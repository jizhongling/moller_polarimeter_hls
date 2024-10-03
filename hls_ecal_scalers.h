#ifndef hls_compton_scalers_h
#define hls_compton_scalers_h

#include <hls_stream.h>
#define AP_INT_MAX_W 4096 // Must be defined before next line
#include <ap_int.h>
#include "hls_moller_ecal.h"

void hls_ecal_scalers(
    hls::stream<vxs_payload_t> &s_vxs_payload,
    hls::stream<trig_t>        &s_trig,
    hls::stream<eventdata_t>   &s_event
    );

#endif
