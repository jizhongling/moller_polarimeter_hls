#include "hls_moller_ecal.h"
#include "hls_ecal.h"
#include "hls_helicity.h"
#include "hls_ecal_scalers.h"

fadc_t get_fadc(vxs_payload_t vxs_payload, int slot, int ch)
{
  /*
     FADC data for each slot is 256 bits, 16 bits per channel (16 channels)
     bit 12-0: hit pulse integral
     bit 15-13: hit time in 32ns frame (4ns resolution)
   */
  int slot_idx = 0;
  if(slot>=3 && slot<=10) slot_idx = slot-3;
  else if(slot>=13 && (slot<=20)) slot_idx = slot-13+8;

  fadc_t result;
  result.e = vxs_payload(slot_idx*256+ch*16+12,slot_idx*256+ch*16+ 0);
  result.t = vxs_payload(slot_idx*256+ch*16+15,slot_idx*256+ch*16+13);

  return result;
}

void vxs_payload_fanout(
    hls::stream<vxs_payload_t> &s_vxs_payload,
    hls::stream<vxs_payload_t> &s_vxs_payload_ecal,
    hls::stream<vxs_payload_t> &s_vxs_payload_helicity,
    hls::stream<vxs_payload_t> &s_vxs_payload_scalers
    )
{
#pragma HLS PIPELINE II=1 style=flp
  vxs_payload_t vxs_payload = s_vxs_payload.read();

  s_vxs_payload_ecal.write(vxs_payload);
  s_vxs_payload_helicity.write(vxs_payload);
  s_vxs_payload_scalers.write(vxs_payload);
}

void trig_fanout(
    hls::stream<trig_t> &s_trig_ecal,
    hls::stream<trig_t> &s_trig_scalers,
    hls::stream<trig_t> &s_trig
    )
{
#pragma HLS PIPELINE II=1 style=flp
  trig_t trig = s_trig_ecal.read();
  s_trig_scalers.write(trig);
  s_trig.write(trig);
}

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
    )
{
#pragma HLS PIPELINE II=1 style=flp
#pragma HLS STABLE variable=fadc_threshold
  static hls::stream<vxs_payload_t> s_vxs_payload_ecal;
  static hls::stream<vxs_payload_t> s_vxs_payload_helicity;
  static hls::stream<vxs_payload_t> s_vxs_payload_scalers;
  static hls::stream<trig_t>        s_trig_ecal;
  static hls::stream<trig_t>        s_trig_scalers;

  vxs_payload_fanout(
      s_vxs_payload,
      s_vxs_payload_ecal,
      s_vxs_payload_helicity,
      s_vxs_payload_scalers
      );

  hls_ecal(
      s_vxs_payload_ecal,
      s_trig_ecal,
      vxs_payload_last,
      fadc_threshold
      );

  trig_fanout(
      s_trig_ecal,
      s_trig_scalers,
      s_trig
      );

  hls_helicity(
      s_vxs_payload_helicity,
      s_trig_accept,
      s_event_helicity,
      helicity_cnt,
      mps_cnt
      );

  hls_ecal_scalers(
      s_vxs_payload_scalers,
      s_trig_scalers,
      s_event_scalers
      );
}
