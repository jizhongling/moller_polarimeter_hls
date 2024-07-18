#include "hls_moller_compton.h"
#include "hls_compton.h"
#include "hls_helicity.h"
#include "hls_compton_scalers.h"

vetroc_t get_vetroc(vxs_payload_t vxs_payload, int slot, ap_uint<3> t)
{
  /*
    VETROC data for each slot is 256 bits, 2 bits per channel (128 channels)
      bit 0: hit in time range 0 to 15.999ns of 32ns frame
      bit 1: hit in time range 16 to 31.999ns of 32ns frame

    For 4ns timing compatibility, 't' can take values from 0 to 7 (0=0ns, 1=4ns, ..., 7=28ns)
      indicating hit time in 32ns frame. A future VETROC firmware update will likely provide
      4ns reporting.
  */
  int slot_idx = 0;
  if(slot>=3 && slot<=10) slot_idx = slot-3;
  else if(slot>=13 && (slot<=20)) slot_idx = slot-13+8;

  vetroc_t result;
  for(int i=0; i<128; i++)
  {
    if(t<4) t = 0;
    else    t = 1;
    result[i] = vxs_payload[slot_idx*256+i*2+t];
  }
  return result;
}

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
    hls::stream<vxs_payload_t> &s_vxs_payload_compton,
    hls::stream<vxs_payload_t> &s_vxs_payload_helicity,
    hls::stream<vxs_payload_t> &s_vxs_payload_scalers
    )
{
#pragma HLS PIPELINE II=1 style=flp
  vxs_payload_t vxs_payload = s_vxs_payload.read();

  s_vxs_payload_compton.write(vxs_payload);
  s_vxs_payload_helicity.write(vxs_payload);
  s_vxs_payload_scalers.write(vxs_payload);
}

void trig_fanout(
    hls::stream<trig_t> &s_trig_compton,
    hls::stream<trig_t> &s_trig_scalers,
    hls::stream<trig_t> &s_trig
  )
{
#pragma HLS PIPELINE II=1 style=flp
  trig_t trig = s_trig_compton.read();
  s_trig_scalers.write(trig);
  s_trig.write(trig);
}

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
  )
{
#pragma HLS STABLE variable=fadc_mask
#pragma HLS STABLE variable=e_plane_mask
#pragma HLS STABLE variable=mult_threshold
#pragma HLS STABLE variable=fadc_threshold
#pragma HLS INTERFACE mode=ap_none port=mps_cnt
#pragma HLS INTERFACE mode=ap_none port=helicity_cnt
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=fadc_mask
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=e_plane_mask
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=mult_threshold
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=fadc_threshold
#pragma HLS DATAFLOW disable_start_propagation
  static hls::stream<vxs_payload_t> s_vxs_payload_compton;
  static hls::stream<vxs_payload_t> s_vxs_payload_helicity;
  static hls::stream<vxs_payload_t> s_vxs_payload_scalers;
  static hls::stream<trig_t>        s_trig_compton;
  static hls::stream<trig_t>        s_trig_scalers;

  vxs_payload_fanout(
      s_vxs_payload,
      s_vxs_payload_compton,
      s_vxs_payload_helicity,
      s_vxs_payload_scalers
    );

  hls_compton(
      s_vxs_payload_compton,
      s_trig_compton,
      fadc_threshold,
      mult_threshold,
      e_plane_mask,
      fadc_mask
    );

  trig_fanout(
      s_trig_compton,
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

  hls_compton_scalers(
      s_vxs_payload_scalers,
      s_trig_scalers,
      s_event_scalers
    );
}
