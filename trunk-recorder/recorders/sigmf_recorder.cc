
#include "sigmf_recorder.h"
#include <boost/log/trivial.hpp>

//static int rec_counter=0;
  
sigmf_recorder_sptr make_sigmf_recorder(Source *src)
{
  return gnuradio::get_initial_sptr(new sigmf_recorder(src));
}

sigmf_recorder::sigmf_recorder(Source *src)
  : gr::hier_block2("sigmf_recorder",
                    gr::io_signature::make(1, 1, sizeof(gr_complex)),
                    gr::io_signature::make(0, 0, sizeof(float))), Recorder("D")
{
  source = src;
  freq   = source->get_center();
  center = source->get_center();
  config = source->get_config();
  qpsk_mod  = source->get_qpsk_mod();
  silence_frames = source->get_silence_frames();
  talkgroup = 0;


  rec_num = rec_counter++;

  state = inactive;

  //double symbol_rate         = 4800;


  timestamp = time(NULL);
  starttime = time(NULL);


  valve = gr::blocks::copy::make(sizeof(gr_complex));
  valve->set_enabled(false);



  //tm *ltm = localtime(&starttime);

 
  int nchars = snprintf(filename, 160, "%ld-%ld_%g.raw", talkgroup,starttime,freq);

  if (nchars >= 160) {
    BOOST_LOG_TRIVIAL(error) << "Analog Recorder: Path longer than 160 charecters";
  }
	raw_sink = gr::blocks::file_sink::make(sizeof(gr_complex), filename);



    connect(self(),               0, valve,                0);
    connect(valve,                0,  raw_sink,             0);

}



sigmf_recorder::~sigmf_recorder() {}


long sigmf_recorder::get_source_count() {
  return 0;
}

Call_Source * sigmf_recorder::get_source_list() {
  return NULL; //wav_sink->get_source_list();
}

Source * sigmf_recorder::get_source() {
  return source;
}

int sigmf_recorder::get_num() {
  return rec_num;
}

bool sigmf_recorder::is_active() {
  if (state == active) {
    return true;
  } else {
    return false;
  }
}

double sigmf_recorder::get_freq() {
  return freq;
}

double sigmf_recorder::get_current_length() {
  return 0; //wav_sink->length_in_seconds();
}

int sigmf_recorder::lastupdate() {
  return time(NULL) - timestamp;
}

long sigmf_recorder::elapsed() {
  return time(NULL) - starttime;
}

void sigmf_recorder::tune_offset(double f) {
 // have to flip this for 3.7
  // BOOST_LOG_TRIVIAL(info) << "Offset set to: " << offset_amount << " Freq: "
  //  << freq;
}

State sigmf_recorder::get_state() {
  return state;
}

void sigmf_recorder::stop() {
  if (state == active) {
    recording_duration += wav_sink->length_in_seconds();
    BOOST_LOG_TRIVIAL(error) << "sigmf_recorder.cc: Stopping Logger \t[ " << rec_num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]";
    state = inactive;
    valve->set_enabled(false);
    raw_sink->close();
  } else {
    BOOST_LOG_TRIVIAL(error) << "sigmf_recorder.cc: Trying to Stop an Inactive Logger!!!";
  }
}

void sigmf_recorder::start(Call *call) {
  if (state == inactive) {
    timestamp = time(NULL);
    starttime = time(NULL);

    talkgroup = call->get_talkgroup();
    freq      = call->get_freq();


    BOOST_LOG_TRIVIAL(info) << "sigmf_recorder.cc: Starting Logger   \t[ " << rec_num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]";


 
	raw_sink->open(call->get_sigmf_filename());
    state = active;
    valve->set_enabled(true);
  } else {
    BOOST_LOG_TRIVIAL(error) << "sigmf_recorder.cc: Trying to Start an already Active Logger!!!";
  }
}
