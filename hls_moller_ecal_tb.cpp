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
  hls::stream<vxs_payload_t> s_vxs_payload;
  hls::stream<trig_t> s_trig;
  hls::stream<trig_t> s_trig_tag;

  cout << "Started C simulation" << endl;

  ifstream fin("/home/jzl/sources/moller_polarimeter_hls/data/ecal.txt");
  string bin_str;
  ap_uint<nt> ecal[nch];
  ap_uint<nt> tag;
  int ien = 0;

  while(fin >> bin_str)
  {
	ap_uint<nt> bin_num = stoi(bin_str, nullptr, 2);
	int index = ien % (nch+1);
	ien++;
	if(index < nch)
	  ecal[index] = bin_num;
	else
	  tag = bin_num;
	if(index != nch)
	  continue;

    vxs_payload_t vxs_payload = 0;
    trig_t trig_tag;
    for(int ch=0; ch<nch; ch++)
    {
      fadc_t fadc;
      fadc.e = 80;
      for(int t=0; t<nt; t++)
    	if(ecal[ch][nt-1-t])
    	{
    	  fadc.t = t;
    	  set_vxs_payload(vxs_payload, fadc, slot, ch);
    	  break;
    	}
    }
    for(int t=0; t<nt; t++)
      trig_tag.n[0][t] = tag[nt-1-t];
    s_vxs_payload.write(vxs_payload);
    s_trig_tag.write(trig_tag);
  }
  fin.close();

  vxs_payload_t vxs_payload_pre = 0;
  while(!s_vxs_payload.empty())
    hls_moller_ecal(s_vxs_payload, s_trig, vxs_payload_pre, eth);

  int event = 0;
  while(!s_trig.empty() and !s_trig_tag.empty())
  {
    trig_t trig = s_trig.read();
    trig_t trig_tag = s_trig_tag.read();
    for(int t=0; t<nt; t++)
      if(trig.n[0][t] != trig_tag.n[0][t])
    	cout << "Event " << event << ": Inconsistent trigger found at time " << t << endl;
    event++;
  }

  cout << "Finished C simulation" << endl;

  return 0;
}
