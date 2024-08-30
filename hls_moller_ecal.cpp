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
    vxs_payload_t &vxs_payload_next,
    ap_uint<13> fadc_threshold
    )
{
#pragma HLS PIPELINE II=1 style=flp
#pragma HLS STABLE variable=fadc_threshold

  // Read 32ns worth of VETROC/FADC trigger data
  vxs_payload_t vxs_payload = vxs_payload_next;
  vxs_payload_next = s_vxs_payload.read();
  trig_t trig;
  for(int t=0; t<nt; t++)
    trig.n[0][t] = 0;

  ap_uint<nt*2> ecal[nch] = {};
  ap_uint<nt*2> trig_tmp = 0;

  for(int ch=0; ch<nch; ch++)
  {
    fadc_t fadc = get_fadc(vxs_payload, slot, ch);
    fadc_t fadc_next = get_fadc(vxs_payload_next, slot, ch);
    // Extend the hit time from the leading edge t to t+dt
    if(fadc.e > eth)
      ecal[ch](fadc.t+nt-1, fadc.t) = -1;
    if(fadc_next.e > eth)
      ecal[ch](2*nt-1, fadc_next.t+nt) = -1;
  }

  // Store the trigger logic for each time
  for(int t=0; t<nt*2; t++)
    trig_tmp[t] = (ecal[0][t] or ecal[1][t] or ecal[2][t] or ecal[3][t]) and (ecal[4][t] or ecal[5][t] or ecal[6][t] or ecal[7][t]);

  // Set the leading edge to 1
  for(int t=0; t<nt; t++)
    trig.n[0][t] = ap_uint<nt*2>(trig_tmp(t+nt-1,t)).or_reduce();

  // Set bits after the leading edge to 0
  for(int t=0; t<nt; t++)
    if(trig.n[0][t] == 1)
      for(int u=1; t+u<nt; u++)
        trig.n[0][t+u] = 0;

  // Write 32ns worth of trigger decisions
  s_trig.write(trig);
}
