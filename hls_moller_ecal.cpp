#include "hls_moller_ecal.h"

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

/* ECal trigger
s_vxs_payload:  FADC and VETROC crate trigger stream
s_trig:         computed trigger bit output/result
 */
void hls_moller_ecal(
    hls::stream<vxs_payload_t> &s_vxs_payload,
    hls::stream<trig_t> &s_trig,
    ap_uint<13> fadc_threshold
    )
{
#pragma HLS PIPELINE II=1 style=flp
#pragma HLS STABLE variable=fadc_threshold

  // Read 32ns worth of VETROC/FADC trigger data
  vxs_payload_t vxs_payload = s_vxs_payload.read();
  trig_t trig;
  for(int t=0; t<nt; t++)
    trig.n[0][t] = 0;

  ap_uint<nch/2> fadc_ecal_l[nt] = {};
  ap_uint<nch/2> fadc_ecal_r[nt] = {};

  for(int ch=0; ch<nch; ch++)
  {
    fadc_t fadc = get_fadc(vxs_payload, slot, ch);
    if(fadc.e > eth)
    {
      if(ch < nch/2)
        fadc_ecal_l[fadc.t][ch] = 1;
      else
        fadc_ecal_r[fadc.t][ch-nch/2] = 1;
    }
  }
  for(int t=0; t<nt; t++)
    trig.n[0][t] = fadc_ecal_l[t].or_reduce() and fadc_ecal_r[t].or_reduce();

  // Write 32ns worth of trigger decisions
  s_trig.write(trig);
}
