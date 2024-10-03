#include "hls_ecal.h"

/* ECal trigger
s_vxs_payload:  FADC crate trigger stream
s_trig:         computed trigger bit output/result
 */
void hls_ecal(
    hls::stream<vxs_payload_t> &s_vxs_payload,
    hls::stream<trig_t> &s_trig,
    vxs_payload_t &vxs_payload_last,
    ap_uint<13> fadc_threshold
    )
{
#pragma HLS PIPELINE II=1 style=flp

  // Read 32ns worth of VETROC/FADC trigger data
  vxs_payload_t vxs_payload = s_vxs_payload.read();
  trig_t trig;
  for(int t=0; t<nt; t++)
    trig.n[0][t] = 0;

  ap_uint<nt*2> ecal[nch] = {};
  ap_uint<nt*2> trig_tmp = 0;

  // Extend the hit time from the leading edge t to t+dt
  for(int ch=0; ch<nch; ch++)
  {
    fadc_t fadc_last = get_fadc(vxs_payload_last, slot, ch);
    if(fadc_last.e > fadc_threshold && !ecal[ch][fadc_last.t])
      for(int u=0; u<dt; u++)
        ecal[ch][fadc_last.t+u] = 1;

    fadc_t fadc = get_fadc(vxs_payload, slot, ch);
    if(fadc.e > fadc_threshold && !ecal[ch][fadc.t+nt])
      for(int u=0; u<dt && fadc.t+u<nt; u++)
        ecal[ch][fadc.t+u+nt] = 1;
  }

  // Store the trigger for each time
  for(int t=0; t<nt*2; t++)
    trig_tmp[t] = (ecal[0][t] or ecal[1][t] or ecal[2][t] or ecal[3][t]) and (ecal[4][t] or ecal[5][t] or ecal[6][t] or ecal[7][t]);

  // Set the leading edge to 1
  for(int t=nt; t<nt*2; t++)
    if(!trig_tmp[t-1] && trig_tmp[t])
      trig.n[0][t-nt] = 1;

  // Write 32ns worth of trigger decisions
  s_trig.write(trig);
  vxs_payload_last = vxs_payload;
}
