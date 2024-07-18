#ifndef hls_compton_scalers_h
#define hls_compton_scalers_h

#include <hls_stream.h>
#include <ap_int.h>
#include "hls_moller_compton.h"

void hls_compton_scalers(
    hls::stream<vxs_payload_t> &s_vxs_payload,
    hls::stream<trig_t>        &s_trig,
    hls::stream<eventdata_t>   &s_event
  );

#endif
