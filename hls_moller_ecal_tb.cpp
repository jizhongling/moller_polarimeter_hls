#include <cstdlib>
#include <iostream>
#include <string>
#include "hls_moller_ecal.h"
using namespace std;

typedef struct
{
  ap_uint<13> e[2][nch];
  ap_uint<3> t[2][nch];
} fadc_ch_t;

void set_vxs_payload(vxs_payload_t &vxs_payload, fadc_t fadc, int slot, int ch)
{
  /*
     FADC data for each slot is 256 bits, 16 bits per channel (16 channels)
     bit 12-0: hit pulse integral
     bit 15-13: hit time in 32ns frame (4ns resolution)
   */
  int slot_idx = 0;
  if(slot>=3 && slot<=10) slot_idx = slot-3;
  else if(slot>=13 && (slot<=20)) slot_idx = slot-13+8;

  vxs_payload(slot_idx*256+ch*16+12,slot_idx*256+ch*16+ 0) = fadc.e;
  vxs_payload(slot_idx*256+ch*16+15,slot_idx*256+ch*16+13) = fadc.t;
}

int main(int argc, char *argv[])
{
  hls::stream<vxs_payload_t> s_vxs_payload;
  hls::stream<trig_t> s_trig;
  hls::stream<trig_t> s_trig_tag;
  hls::stream<fadc_ch_t> s_fadc;

  cout << "Started C simulation" << endl;

  int nframe = argc > 1 ? stoi(argv[1]) : 10;
  for(int i=0; i<nframe; i++)
  {
    vxs_payload_t vxs_payload = 0;
    trig_t trig_tag;
    fadc_ch_t fadc_ch;
    ap_uint<nt*2> ecal[nch] = {};
    ap_uint<nt*2> trig_tmp = 0;
    for(int t=0; t<nt; t++)
      trig_tag.n[0][t] = 0;
    for(int ch=0; ch<nch; ch++)
    {
      fadc_ch.e[0][ch] = i==0 ? ap_uint<13>(rand() % emax) : fadc_ch.e[1][ch];
      fadc_ch.e[1][ch] = rand() % emax;
      fadc_ch.t[0][ch] = i==0 ? ap_uint<3>(rand() % nt) : fadc_ch.t[1][ch];
      fadc_ch.t[1][ch] = rand() % nt;
      if(fadc_ch.e[0][ch] > eth)
        ecal[ch](fadc_ch.t[0][ch]+nt-1, fadc_ch.t[0][ch]) = -1;
      if(fadc_ch.e[1][ch] > eth)
        ecal[ch](nt*2-1, fadc_ch.t[1][ch]+nt) = -1;
      fadc_t fadc;
      fadc.e = fadc_ch.e[0][ch];
      fadc.t = fadc_ch.t[0][ch];
      set_vxs_payload(vxs_payload, fadc, slot, ch);
    }
    for(int t=0; t<nt*2; t++)
      trig_tmp[t] = (ecal[0][t] or ecal[1][t] or ecal[2][t] or ecal[3][t]) and (ecal[4][t] or ecal[5][t] or ecal[6][t] or ecal[7][t]);
    for(int t=0; t<nt; t++)
      trig_tag.n[0][t] = ap_uint<nt*2>(trig_tmp(t+nt-1, t)).or_reduce();
    // Set bits after the leading edge to 0
    for(int t=0; t<nt; t++)
      if(trig_tag.n[0][t] == 1)
        for(int u=1; t+u<nt; u++)
          trig_tag.n[0][t+u] = 0;
    s_vxs_payload.write(vxs_payload);
    s_trig_tag.write(trig_tag);
    s_fadc.write(fadc_ch);
  }

  vxs_payload_t vxs_payload_next;
  if(!s_vxs_payload.empty())
    vxs_payload_next = s_vxs_payload.read();
  while(!s_vxs_payload.empty())
    hls_moller_ecal(s_vxs_payload, s_trig, vxs_payload_next, eth);

  int event = 0;
  while(!s_trig.empty() and !s_trig_tag.empty() and !s_fadc.empty())
  {
    trig_t trig = s_trig.read();
    trig_t trig_tag = s_trig_tag.read();
    fadc_ch_t fadc_ch = s_fadc.read();
    cout << "\nEvent " << event++ << endl;
    for(int i=0; i<2; i++)
    {
      cout << "energy" << i << ":";
      for(int ch=0; ch<nch; ch++)
        cout << "\t" << fadc_ch.e[i][ch];
      cout << "\nt_start" << i << ":";
      for(int ch=0; ch<nch; ch++)
        cout << "\t" << fadc_ch.t[i][ch];
      cout << endl;
    }
    cout << "t\t\ttrig\ttrig_tag\n";
    for(int t=0; t<nt; t++)
    {
      cout << t << "\t\t" << trig.n[0][t] << "\t\t" << trig_tag.n[0][t] << endl;
      if(trig.n[0][t] != trig_tag.n[0][t])
        cout << "Inconsistent trigger found" << endl;
    }
  }

  cout << "Finished C simulation" << endl;

  return 0;
}
