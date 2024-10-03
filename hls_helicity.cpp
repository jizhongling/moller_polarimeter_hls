#include "hls_helicity.h"

#define S_WRITE0    0
#define S_WRITE1    1
#define S_WRITE2    2
#define S_WRITE3    3
#define S_WRITE4    4
#define S_WRITE5    5
#define S_WRITE6    6
#define S_WRITE7    7
#define S_WRITE_END 8

/* Helicity recording logic
     s_vxs_payload:  FADC and VETROC crate trigger stream
     s_trig_accept:  Trigger window for each accepted DAQ trigger (i.e. L1A)
     s_event:        Event data stream contain helicity info built in this function
     helicity_cnt:   Diagnostic register to monitor the number of 31.25MHz clock cycles helicity signal is high
     mps_cnt:        Diagnostic register to monitor the number of 31.25MHz clock cycles mps signal is high
*/
void hls_helicity(
    hls::stream<vxs_payload_t> &s_vxs_payload,
    hls::stream<trig_accept_t> &s_trig_accept,
    hls::stream<eventdata_t>   &s_event,
    ap_uint<32>                &helicity_cnt,
    ap_uint<32>                &mps_cnt
  )
{
#pragma HLS PIPELINE II=1 style=flp
  static bool mps_last;
  static ap_uint<32> helicity_cnt_i = 0;
  static ap_uint<32> mps_cnt_i = 0;
  static ap_uint<31> mps_event_time = 0;
  static ap_uint<31> mps_last_time = 0;
  static ap_uint<31> mps_window_count = 0;
  static ap_uint<31> mps_window_count_event = 0;
  static ap_uint<173> helicity_last = 0;
  static ap_uint<173> helicity_last_evt;
  static ap_uint<4> trig_state = S_WRITE0;
  bool mps, helicity;
  eventdata_t eventdata;

  // Read 32ns worth of VETROC/FADC trigger data
  vxs_payload_t vxs_payload = s_vxs_payload.read();

  // 32ns frame of discriminated MPS
  fadc_t fadc_mps = get_fadc(vxs_payload, 3, 8);
  mps      = fadc_mps.e > 0 ? true : false;

  // 32ns frame of discriminated helicity
  fadc_t fadc_helicity = get_fadc(vxs_payload, 3, 9);
  helicity = fadc_helicity.e > 0 ? true : false;

  if(helicity)
  {
    helicity_cnt_i++;
    helicity_cnt = helicity_cnt_i;
  }
  if(mps)
  {
    mps_cnt_i++;
    mps_cnt = mps_cnt_i;
  }

  if(!mps_last && mps)  // detect MPS rising edge
  {
    mps_window_count++;
    mps_last_time = 0;
    helicity_last = (helicity_last(172,0), helicity);
  }

  switch(trig_state)
  {
    case S_WRITE0:
    {
      // check if we have a trigger and need to build an event
      if(!s_trig_accept.empty())
      {
        // read & throw away trigger readout window - don't really have a typical readout window here
        s_trig_accept.read();

        helicity_last_evt = helicity_last;
        mps_event_time = mps_last_time;
        mps_window_count_event = mps_window_count;

        // Event word 0
        eventdata.data(31,31) = 1;                    // Data defining tag
        eventdata.data(30,27) = 12;                   // Extended tag type
        eventdata.data(26,23) = 8;                    // Helicity info type
        eventdata.data(22, 0) = helicity_last_evt(22,0);  // Record last 23 helicity samples
        eventdata.end = 0;
        s_event.write(eventdata);

        trig_state = S_WRITE1;
      }
      break;
    }

    case S_WRITE1:
    {
      // Event word 1
      eventdata.data(31,31) = 0;                // Data continuation
      eventdata.data(30, 0) = mps_event_time;   // last MPS timestamp
      eventdata.end = 0;
      s_event.write(eventdata);

      trig_state = S_WRITE2;
      break;
    }

    case S_WRITE2:
    {
      // Event word 2
      eventdata.data(31,31) = 0;                // Data continuation
      eventdata.data(30, 0) = mps_window_count_event; // MPS window counter
      eventdata.end = 0;
      s_event.write(eventdata);

      trig_state = S_WRITE3;
      break;
    }

    case S_WRITE3:
    {
      // Event word 3
      eventdata.data(31,31) = 0;                    // Data continuation
      eventdata.data(30, 0) = helicity_last_evt(52,23); // additional previous helicity samples
      eventdata.end = 0;
      s_event.write(eventdata);

      trig_state = S_WRITE4;
      break;
    }

    case S_WRITE4:
    {
      // Event word 4
      eventdata.data(31,31) = 0;                    // Data continuation
      eventdata.data(30, 0) = helicity_last_evt(82,53); // additional previous helicity samples
      eventdata.end = 0;
      s_event.write(eventdata);

      trig_state = S_WRITE5;
      break;
    }

    case S_WRITE5:
    {
      // Event word 5
      eventdata.data(31,31) = 0;                    // Data continuation
      eventdata.data(30, 0) = helicity_last_evt(112,83); // additional previous helicity samples
      eventdata.end = 0;
      s_event.write(eventdata);

      trig_state = S_WRITE6;
      break;
    }

    case S_WRITE6:
    {
      // Event word 6
      eventdata.data(31,31) = 0;                    // Data continuation
      eventdata.data(30, 0) = helicity_last_evt(142,113); // additional previous helicity samples
      eventdata.end = 0;
      s_event.write(eventdata);

      trig_state = S_WRITE7;
      break;
    }

    case S_WRITE7:
    {
      // Event word 7
      eventdata.data(31,31) = 0;                    // Data continuation
      eventdata.data(30, 0) = helicity_last_evt(172,143); // additional previous helicity samples
      eventdata.end = 0;
      s_event.write(eventdata);

      trig_state = S_WRITE_END;
      break;
    }

    case S_WRITE_END:
    {
      // End of event
      eventdata.data(31,0) = 0xFFFFFFFF;
      eventdata.end = 1;
      s_event.write(eventdata);

      trig_state = S_WRITE0;
      break;
    }
  }

  mps_last_time+= 8;  //+8*4ns
  mps_last = mps;
}
