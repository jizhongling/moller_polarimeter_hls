#include "hls_compton_scalers.h"

int active_vetroc_channels[96] = {
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
     31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
     64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87,
     96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119
    };

void init_mem_active_vetroc_channels(int array[96])
{
  for(int i=0;i<96;i++)
    array[i] = active_vetroc_channels[i];
}

typedef struct
{
  ap_uint<19> Strips[4][96];
  ap_uint<31> BusyTime;
  ap_uint<31> LiveTime;
  ap_uint<31> Trig[5];
  ap_uint<31> MPSWindow;
} ComptonScalers_t;


/* Scaler recording logic
     s_vxs_payload:  FADC and VETROC crate trigger stream
     s_trig:         Trigger decision from hls_compton trigger logic
     s_event:        (ASYNC) Event data stream contain scaler info built in this function.

     - This function builds this scaler event for each new helicity cycle.
     - If the output buffer is full this event will not be generated.
     - A large output buffer to the event builder needed to ensure these events are always
       written/recorded (0 dead-time goal). If there is dead-time the event keeps track of
       the helicity window number with MPSWindow so a missed window will be known.
*/
void hls_compton_scalers(
    hls::stream<vxs_payload_t> &s_vxs_payload,
    hls::stream<trig_t>        &s_trig,
    hls::stream<eventdata_t>   &s_event
  )
{
#pragma HLS PIPELINE II=1 style=flp
  static bool mps_last = true;
  static ap_uint<5> trig_last;
  static ComptonScalers_t Scalers;
  static ComptonScalers_t Scalers_Buffered;
  static bool evtbuild_active = false;
  static int evtbuild_pos;
  bool mps, busy, mps_rise, start = false;
  bool event_stream_ready = false;
  ap_uint<5> trig_inc = 0;
  trig_t trig;
  eventdata_t eventdata;

  if(!s_trig.empty())
  {
    trig = s_trig.read();

    ap_uint<5> current_hit;
    for(int i=0;i<5;i++)
    {
      current_hit[i] = trig.n[i][0] | trig.n[i][1] | trig.n[i][2] | trig.n[i][3] |
                       trig.n[i][4] | trig.n[i][5] | trig.n[i][6] | trig.n[i][7];

      if( (current_hit[i]==1) && (trig_last[i]==0) )
        trig_inc[i] = 1;
    }
    trig_last = current_hit;
  }

  // Read 32ns worth of VETROC/FADC trigger data
  vxs_payload_t vxs_payload = s_vxs_payload.read();

  // 32ns frame of discriminated MPS
  fadc_t fadc_mps = get_fadc(vxs_payload, 3, 1);
  mps      = fadc_mps.e > 0 ? true : false;

  // 32ns frame of discriminated busy
  fadc_t fadc_busy = get_fadc(vxs_payload, 3, 4);
  busy     = fadc_busy.e > 0 ? true : false;

  // Detect leading edge of MPS
  mps_rise = mps && !mps_last;

  if(mps_rise)
    Scalers.MPSWindow++;

  if(mps)
  {
    Scalers.BusyTime = 0;
    Scalers.LiveTime = 0;
  }
  else if(busy)
    Scalers.BusyTime+= 8;
  else
    Scalers.LiveTime+= 8;

  for(int i=0;i<5;i++)
  {
    if(mps)
      Scalers.Trig[i] = 0;
    else if(trig_inc[i])
      Scalers.Trig[i]++;
  }

  for(int i=0;i<4;i++)
  {
    vetroc_t vetroc = get_vetroc(vxs_payload,13+i,0) | get_vetroc(vxs_payload,13+i,4);

    for(int j=0;j<96;j++)
    {
      int ch = active_vetroc_channels[j];
      ap_uint<1> inc = vetroc[ch];

      if(mps)
        Scalers.Strips[i][j] = 0;
      else if(inc)
        Scalers.Strips[i][j]++;
    }
  }

  if(!evtbuild_active && mps_rise)
  {
    for(int i=0;i<4;i++)
    for(int j=0;j<96;j++)
      Scalers_Buffered.Strips[i][j] = Scalers.Strips[i][j];

    Scalers_Buffered.BusyTime = Scalers.BusyTime;
    Scalers_Buffered.LiveTime = Scalers.LiveTime;

    for(int i=0;i<5;i++)
      Scalers_Buffered.Trig[i] = Scalers.Trig[i];

    Scalers_Buffered.MPSWindow = Scalers.MPSWindow;

    evtbuild_active = true;
  }
  else if(evtbuild_active)
  {
    if(evtbuild_pos==0)
    {
      eventdata.data(31,31) = 1;  // Data defining tag
      eventdata.data(30,27) = 12; // Extended tag type
      eventdata.data(26,23) = 9;  // Compton trigger scalers type
      eventdata.data(22, 0) = 0;  // not used
      eventdata.end = 0;
    }
    else if(evtbuild_pos==1)
    {
      eventdata.data(31,31) = 0;
      eventdata.data(30, 0) = Scalers_Buffered.MPSWindow;
      eventdata.end = 0;
    }
    else if(evtbuild_pos==2)
    {
      eventdata.data(31,31) = 0;
      eventdata.data(30, 0) = Scalers_Buffered.LiveTime;
      eventdata.end = 0;
    }
    else if(evtbuild_pos==3)
    {
      eventdata.data(31,31) = 0;
      eventdata.data(30, 0) = Scalers_Buffered.BusyTime;
      eventdata.end = 0;
    }
    else if((evtbuild_pos>=4) && (evtbuild_pos<=8))
    {
      eventdata.data(31,31) = 0;
      eventdata.data(30, 0) = Scalers_Buffered.Trig[evtbuild_pos-4];
      eventdata.end = 0;
    }
    // Slot 13 strip scalers
    else if(evtbuild_pos==9)
    {
      eventdata.data(31,31) = 1;  // Data defining tag
      eventdata.data(30,27) = 12; // Extended tag type
      eventdata.data(26,23) = 9;  // Compton strip scalers type
      eventdata.data(22, 0) = 13; // slot
      eventdata.end = 0;
    }
    else if((evtbuild_pos>=10) && (evtbuild_pos<=105))
    {
      eventdata.data(31,31) = 0;
      eventdata.data(30,24) = active_vetroc_channels[evtbuild_pos-10];
      eventdata.data(23, 0) = Scalers_Buffered.Strips[0][evtbuild_pos-10];
      eventdata.end = 0;
    }
    // Slot 14 strip scalers
    else if(evtbuild_pos==106)
    {
      eventdata.data(31,31) = 1;  // Data defining tag
      eventdata.data(30,27) = 12; // Extended tag type
      eventdata.data(26,23) = 9;  // Compton strip scalers type
      eventdata.data(22, 0) = 14; // slot
      eventdata.end = 0;
    }
    else if((evtbuild_pos>=107) && (evtbuild_pos<=202))
    {
      eventdata.data(31,31) = 0;
      eventdata.data(30,24) = active_vetroc_channels[evtbuild_pos-107];
      eventdata.data(23, 0) = Scalers_Buffered.Strips[1][evtbuild_pos-107];
      eventdata.end = 0;
    }
    // Slot 15 strip scalers
    else if(evtbuild_pos==203)
    {
      eventdata.data(31,31) = 1;  // Data defining tag
      eventdata.data(30,27) = 12; // Extended tag type
      eventdata.data(26,23) = 9;  // Compton strip scalers type
      eventdata.data(22, 0) = 15; // slot
      eventdata.end = 0;
    }
    else if((evtbuild_pos>=204) && (evtbuild_pos<=299))
    {
      eventdata.data(31,31) = 0;
      eventdata.data(30,24) = active_vetroc_channels[evtbuild_pos-204];
      eventdata.data(23, 0) = Scalers_Buffered.Strips[2][evtbuild_pos-204];
      eventdata.end = 0;
    }
    // Slot 16 strip scalers
    else if(evtbuild_pos==300)
    {
      eventdata.data(31,31) = 1;  // Data defining tag
      eventdata.data(30,27) = 12; // Extended tag type
      eventdata.data(26,23) = 9;  // Compton strip scalers type
      eventdata.data(22, 0) = 16; // slot
      eventdata.end = 0;
    }
    else if((evtbuild_pos>=301) && (evtbuild_pos<=396))
    {
      eventdata.data(31,31) = 0;
      eventdata.data(30,24) = active_vetroc_channels[evtbuild_pos-301];
      eventdata.data(23, 0) = Scalers_Buffered.Strips[3][evtbuild_pos-301];
      eventdata.end = 0;
    }
    else
    {
      // End of event
      eventdata.data(31,0) = 0xFFFFFFFF;
      eventdata.end = 1;
    }
    s_event.write(eventdata);

    if(eventdata.end == 1)
    {
      evtbuild_pos = 0;
      evtbuild_active = false;
    }
    else
      evtbuild_pos++;
  }

  mps_last = mps;
}
