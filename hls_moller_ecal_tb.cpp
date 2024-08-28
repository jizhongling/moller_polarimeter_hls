#include <cstdlib>
#include <iostream>
#include <string>
#include "hls_moller_ecal.h"
using namespace std;

typedef struct
{
  ap_uint<13> n[nch];
} e_t;

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
  hls::stream<e_t> s_e;

  cout << "Started C simulation" << endl;

  int nframe = argc > 1 ? stoi(argv[1]) : 10;
  for(int i=0; i<nframe; i++)
  {
    vxs_payload_t vxs_payload = 0;
    trig_t trig_tag;
    for(int t=0; t<nt; t++)
      trig_tag.n[0][t] = 0;
    e_t e;
    int t = rand() % nt;
    for(int ch=0; ch<nch; ch++)
    {
      e.n[ch] = rand() % emax;
      fadc_t fadc;
      fadc.e = e.n[ch];
      fadc.t = t;
      set_vxs_payload(vxs_payload, fadc, slot, ch);
    }
    trig_tag.n[0][t] = (e.n[0]>eth or e.n[1]>eth or e.n[2]>eth or e.n[3]>eth) and (e.n[4]>eth or e.n[5]>eth or e.n[6]>eth or e.n[7]>eth);
    s_vxs_payload.write(vxs_payload);
    s_trig_tag.write(trig_tag);
    s_e.write(e);
  }

  while(!s_vxs_payload.empty())
    hls_moller_ecal(s_vxs_payload, s_trig, eth);

  while(!s_trig.empty() and !s_trig_tag.empty() and !s_e.empty())
  {
    trig_t trig = s_trig.read();
    trig_t trig_tag = s_trig_tag.read();
    e_t e = s_e.read();
    for(int t=0; t<nt; t++)
    {
      if(trig.n[0][t] == trig_tag.n[0][t])
      {
        cout << "Consistent trigger found" << endl;
      }
      else
      {
        cout << "Inconsistent trigger found" << endl;
        cout << "e: " << e.n[0] << ", " << e.n[1] << ", " << e.n[2] << ", " << e.n[3] << ", "
          << e.n[4] << ", " << e.n[5] << ", " << e.n[6] << ", " << e.n[7] << endl;
        cout << "trig: " << trig.n[0][t] << ", trig_tag: " << trig_tag.n[0][t] << endl;
      }
    }
  }

  cout << "Finished C simulation" << endl;

  return 0;
}
