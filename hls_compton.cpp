#include "hls_compton.h"

ap_uint<3> calc_mult(
    ap_uint<1> a,
    ap_uint<1> b,
    ap_uint<1> c,
    ap_uint<1> d
  )
{
  ap_uint<3> mult = 0;
  if(a) mult++;
  if(b) mult++;
  if(c) mult++;
  if(d) mult++;
  return mult;
}

/* Electron plane and photon trigger logic
     s_vxs_payload:  FADC and VETROC crate trigger stream
     s_trig:         computed trigger bit output/result
     fadc_threshold: photon detector threshold (applied to pedestal subtracted pulse integral with gain computed on FADC)
     mult_threshold: electron plane multiplicity requirement for each trigger bit
     e_plane_mask:   electron plane mask that chooses which planes are enable for multiplicity check
     fadc_mask:      chooses which FADC channel is used for the photo detector input
*/
void hls_compton(
    hls::stream<vxs_payload_t> &s_vxs_payload,
    hls::stream<trig_t> &s_trig,
    ap_uint<13> fadc_threshold[5],
    ap_uint<3> mult_threshold[5],
    ap_uint<4> e_plane_mask[5],
    ap_uint<16> fadc_mask[5]
  )
{
#pragma HLS PIPELINE II=1 style=flp
  // Read 32ns worth of VETROC/FADC trigger data
  vxs_payload_t vxs_payload = s_vxs_payload.read();

  ap_uint<8> fadc_hit[5] = {0,0,0,0,0};
  ap_uint<8> e_plane_hit[4] = {0,0,0,0};
  for(int t=0; t<8; t++)
  {
    for(int plane=0; plane<4; plane++)
    {
      // Get VETROC slot data: plane 0 => slot13, 1 => slot14, ..., 3 => slot 16
      vetroc_t vetroc = get_vetroc(vxs_payload, plane+13, t);
      // Check VETROC electron planes for hits
      e_plane_hit[plane][t] = vetroc.or_reduce();
    }
  }

  for(int trg=0; trg<5; trg++)
  {
    // Check all channels of first FADC slot for a pulse over threshold
    for(int ch=0; ch<16; ch++)
    {
      fadc_t fadc = get_fadc(vxs_payload, 3, ch);
      if( (fadc.e >= fadc_threshold[trg]) && (fadc_mask[trg][ch] == 1) )
        fadc_hit[trg][fadc.t] = 1;
    }
  }

  // Form trigger decisions (8 decisions made for each 4ns time in this 32ns window of data)
  trig_t trig;
  for(int t=0; t<8; t++)
  {
    for(int trg=0; trg<5; trg++)
    {
      ap_uint<3> mult = calc_mult(
          e_plane_hit[0][t] & e_plane_mask[trg][0],
          e_plane_hit[1][t] & e_plane_mask[trg][1],
          e_plane_hit[2][t] & e_plane_mask[trg][2],
          e_plane_hit[3][t] & e_plane_mask[trg][3]
        );
      if((!fadc_mask[trg] || fadc_hit[trg][t]) && (mult >= mult_threshold[trg]))
        trig.n[trg][t] = 1;
      else
        trig.n[trg][t] = 0;
    }
  }

  // Write 32ns worth of trigger decisions
  s_trig.write(trig);
}
