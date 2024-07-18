#ifndef hls_compton_h
#define hls_compton_h

#include <hls_stream.h>
#include <ap_int.h>
#include "hls_moller_compton.h"

// hls_compton:
//   - s_vxs_payload: contains 16 slots of VXS payload information updated every 32ns
//     for compton we assume the following crate configuration:
//       FADC   in slot 3  photon detector
//       VETROC in slot 13 electron detector plane 1
//       VETROC in slot 14 electron detector plane 2
//       VETROC in slot 15 electron detector plane 3
//       VETROC in slot 16 electron detector plane 4
//
//   - s_trig: contains trigger bit decisions, currently 5 triggers:
//       s_trig.n[0] is photon AND mult(electron_layer_1,electron_layer_2,electron_layer_3,,electron_layer_4)>=MULT_THR
//       s_trig.n[1] is electron_layer_1
//       s_trig.n[2] is electron_layer_2
//       s_trig.n[3] is electron_layer_3
//       s_trig.n[4] is electron_layer_4
//
//   - Currently this code expects:
//       1) VETROC hit pulse widths to be defined by VERTOC module hit width (on VETROC)
//       2) FADC hits define the trigger time (i.e. no width)
//       3) FADC hit delays are defined by FADC module (a 128ns programmable delay)
//
//     A VETROC programmable delay can be added if needed (and/or FADC programmable delay range can be increased)
void hls_compton(
    hls::stream<vxs_payload_t> &s_vxs_payload,
    hls::stream<trig_t> &s_trig,
    ap_uint<13> fadc_threshold[5],
    ap_uint<3> mult_threshold[5],
    ap_uint<4> e_plane_mask[5],
    ap_uint<16> fadc_mask[5]
  );

#endif
