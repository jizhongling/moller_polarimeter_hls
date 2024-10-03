#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include "hls_moller_ecal.h"
using namespace std;

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
  // FADC Streaming (input)
  hls::stream<vxs_payload_t> s_vxs_payload;
  hls::stream<trig_t> s_trig;
  hls::stream<trig_t> s_trig_tag;

  // ECal trigger parameters & trigger bit stream (output)
  vxs_payload_t vxs_payload_last = 0;
  ap_uint<13> fadc_threshold = 0;

  // Helicity event builder
  hls::stream<eventdata_t>    s_event_helicity;
  hls::stream<trig_accept_t>  s_trig_accept;
  ap_uint<32>                 helicity_cnt = 0;
  ap_uint<32>                 mps_cnt = 0;

  // Helicity scalers event builder
  hls::stream<eventdata_t>    s_event_scalers;

  cout << "\nStarted C simulation\n";

  ifstream fin("/home/jzl/sources/moller_polarimeter_hls/data/ecal.txt");
  string bin_str;
  vxs_payload_t vxs_payload;
  int ien = 0;

  // Fill HLS stream
  while(fin >> bin_str)
  {
    ap_uint<nt> bin_num = stoi(bin_str, nullptr, 2);
    int index = ien % (nch+2);
    ien++;

    if(index == 0)
      vxs_payload = 0;

    if(index < nch)
    {
      ap_uint<nt> ecal = bin_num(0, nt-1);  // bit-reversed for (Lo,Hi) instead of (Hi,Lo)
      int ch = index;
      fadc_t fadc_ecal;
      fadc_ecal.e = 80;
      for(int t=0; t<nt; t++)
        if(ecal[t])
        {
          fadc_ecal.t = t;
          set_vxs_payload(vxs_payload, fadc_ecal, slot, ch);
          break;
        }
    }
    else if(index == nch)
    {
      ap_uint<nt> tag = bin_num(0, nt-1);  // bit-reversed for (Lo,Hi) instead of (Hi,Lo)
      trig_t trig_tag;
      for(int t=0; t<nt; t++)
        trig_tag.n[0][t] = tag[t];
      s_trig_tag.write(trig_tag);
    }
    else if(index == nch+1)
    {
      ap_uint<3> t32 = bin_num(0, 2);  // bit-reversed for (Lo,Hi) instead of (Hi,Lo)
      for(int ch=0; ch<3; ch++)
      {
        fadc_t fadc_t32;
        fadc_t32.e = t32[ch] ? 80 : 0;
        fadc_t32.t = 0;
        // CH8: MPS, CH9: Helicity, CH10: Busy
        set_vxs_payload(vxs_payload, fadc_t32, slot, ch+8);
      }
      trig_accept_t trig_accept;
      s_trig_accept.write(trig_accept);
      s_vxs_payload.write(vxs_payload);
    }
  }
  fin.close();

  // Run HLS code
  while(!s_vxs_payload.empty())
    hls_moller_ecal(s_vxs_payload, s_trig, vxs_payload_last, fadc_threshold,
        s_event_helicity, s_trig_accept, helicity_cnt, mps_cnt, s_event_scalers);

  // Verify trigger outputs
  int event = 0;
  cout << "\nInconsistent triggers:\n";
  while(!s_trig.empty() and !s_trig_tag.empty())
  {
    trig_t trig = s_trig.read();
    trig_t trig_tag = s_trig_tag.read();
    for(int t=0; t<nt; t++)
      if(trig.n[0][t] != trig_tag.n[0][t])
        cout << "Event " << event << ": Inconsistent trigger found at time " << t << endl;
    event++;
  }

  // Print helicity outputs
  cout << "\nHelicity outputs:\n";
  while(!s_event_helicity.empty())
  {
    eventdata_t data_hel = s_event_helicity.read();
    cout << hex << data_hel.data << "\t" << data_hel.end << endl;
  }

  // Print scaler outputs
  cout << "\nScalers outputs:\n";
  while(!s_event_scalers.empty())
  {
    eventdata_t data_scalers = s_event_scalers.read();
    cout << hex << data_scalers.data << "\t" << data_scalers.end << endl;
  }

  cout << "\nFinished C simulation\n\n";

  return 0;
}
