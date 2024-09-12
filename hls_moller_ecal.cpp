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
    vxs_payload_t &vxs_payload_pre,
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

  ap_uint<nt*2> ecal[nch] = {};
  ap_uint<4> hits_or[nt*2] = {};

  // Extend the hit time from the leading edge t to t+dt
  for(int ch=0; ch<nch; ch++)
  {
    fadc_t fadc_pre = get_fadc(vxs_payload_pre, slot, ch);
    if(fadc_pre.e > eth && !ecal[ch][fadc_pre.t])
      for(int u=0; u<dt; u++)
        ecal[ch][fadc_pre.t+u] = 1;

    fadc_t fadc = get_fadc(vxs_payload, slot, ch);
    if(fadc.e > eth && !ecal[ch][fadc.t+nt])
      for(int u=0; u<dt && fadc.t+u<nt; u++)
        ecal[ch][fadc.t+u+nt] = 1;
  }

  // Store the trigger multiplicity for each time
  for(int t=0; t<nt*2; t++)
	for(int ch=0; ch<nch; ch++)
      hits_or[t] += ecal[ch][t];

  // Set the leading edge to 1
  for(int t=nt; t<nt*2; t++)
  	if(hits_or[t-1] < nth && hits_or[t] >= nth)
        trig.n[0][t-nt] = 1;

  // Write 32ns worth of trigger decisions
  s_trig.write(trig);
  vxs_payload_pre = vxs_payload;
}
