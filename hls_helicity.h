#ifndef hls_helicity_h
#define hls_helicity_h

#include <hls_stream.h>
#include <ap_int.h>
#include "hls_moller_compton.h"

void hls_helicity(
    hls::stream<vxs_payload_t> &s_vxs_payload,
    hls::stream<trig_accept_t> &s_trig_accept,
    hls::stream<eventdata_t>   &s_event,
    ap_uint<32>                &helicity_cnt,
    ap_uint<32>                &mps_cnt
  );

#endif
