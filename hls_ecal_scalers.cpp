#include "hls_ecal_scalers.h"

typedef struct
{
  ap_uint<31> BusyTime;
  ap_uint<31> LiveTime;
  ap_uint<31> Trig[1];
  ap_uint<31> MPSWindow;
} ECalScalers_t;


/* Scaler recording logic
s_vxs_payload:  FADC crate trigger stream
s_trig:         Trigger decision from hls_compton trigger logic
s_event:        (ASYNC) Event data stream contain scaler info built in this function.

- This function builds this scaler event for each new helicity cycle.
- If the output buffer is full this event will not be generated.
- A large output buffer to the event builder needed to ensure these events are always
written/recorded (0 dead-time goal). If there is dead-time the event keeps track of
the helicity window number with MPSWindow so a missed window will be known.
 */
void hls_ecal_scalers(
    hls::stream<vxs_payload_t> &s_vxs_payload,
    hls::stream<trig_t>        &s_trig,
    hls::stream<eventdata_t>   &s_event
    )
{
#pragma HLS PIPELINE II=1 style=flp
  static bool mps_last = true;
  static ap_uint<5> trig_last;
  static ECalScalers_t Scalers;
  static ECalScalers_t Scalers_Buffered;
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
    for(int i=0;i<1;i++)
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
  fadc_t fadc_mps = get_fadc(vxs_payload, slot, 8);
  mps      = fadc_mps.e > 0 ? true : false;

  // 32ns frame of discriminated busy
  fadc_t fadc_busy = get_fadc(vxs_payload, slot, 10);
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

  for(int i=0;i<1;i++)
  {
    if(mps)
      Scalers.Trig[i] = 0;
    else if(trig_inc[i])
      Scalers.Trig[i]++;
  }

  if(!evtbuild_active)
  {
    Scalers_Buffered.BusyTime = Scalers.BusyTime;
    Scalers_Buffered.LiveTime = Scalers.LiveTime;

    for(int i=0;i<1;i++)
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
    else if((evtbuild_pos>=4) && (evtbuild_pos<=4))
    {
      eventdata.data(31,31) = 0;
      eventdata.data(30, 0) = Scalers_Buffered.Trig[evtbuild_pos-4];
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
