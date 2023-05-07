
#include "sigmf_recorder_impl.h"
#include <boost/log/trivial.hpp>

// static int rec_counter=0;

sigmf_recorder_sptr make_sigmf_recorder(Source *src) {
  sigmf_recorder *recorder = new sigmf_recorder_impl(src);

  return gnuradio::get_initial_sptr(recorder);
}

sigmf_recorder_impl::sigmf_recorder_impl(Source *src)
    : gr::hier_block2("sigmf_recorder",
                      gr::io_signature::make(1, 1, sizeof(gr_complex)),
                      gr::io_signature::make(0, 0, sizeof(float))),
      Recorder(SIGMF,src->get_config()) {
  source = src;
  freq = source->get_center();
  center = source->get_center();
  config = source->get_config();
  silence_frames = source->get_silence_frames();
  talkgroup = 0;
  recording_count = 0;
  recording_duration = 0;

  rec_num = rec_counter++;

  state = INACTIVE;

  // double symbol_rate         = 4800;

  timestamp = time(NULL);
  starttime = time(NULL);

  valve = gr::blocks::copy::make(sizeof(gr_complex));
  valve->set_enabled(false);

  // tm *ltm = localtime(&starttime);

  int nchars = snprintf(filename, 160, "%ld-%ld_%g.raw", talkgroup, starttime, freq);

  if (nchars >= 160) {
    BOOST_LOG_TRIVIAL(error) << "Analog Recorder: Path longer than 160 charecters";
  }
  raw_sink = gr::blocks::file_sink::make(sizeof(gr_complex), filename);

  connect(self(), 0, valve, 0);
  connect(valve, 0, raw_sink, 0);
}

int sigmf_recorder_impl::get_num() {
  return rec_num;
}

bool sigmf_recorder_impl::is_active() {
  if (state == ACTIVE) {
    return true;
  } else {
    return false;
  }
}

double sigmf_recorder_impl::get_freq() {
  return freq;
}

double sigmf_recorder_impl::get_current_length() {
  return 0;
}

int sigmf_recorder_impl::lastupdate() {
  return time(NULL) - timestamp;
}

long sigmf_recorder_impl::elapsed() {
  return time(NULL) - starttime;
}

void sigmf_recorder_impl::tune_offset(double f) {
  // have to flip this for 3.7
  // BOOST_LOG_TRIVIAL(info) << "Offset set to: " << offset_amount << " Freq: "
  //  << freq;
}

State sigmf_recorder_impl::get_state() {
  return state;
}

void sigmf_recorder_impl::stop() {
  if (state == ACTIVE) {
    BOOST_LOG_TRIVIAL(error) << "sigmf_recorder.cc: Stopping Logger \t[ " << rec_num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]";
    state = INACTIVE;
    valve->set_enabled(false);
    raw_sink->close();
  } else {
    BOOST_LOG_TRIVIAL(error) << "sigmf_recorder.cc: Trying to Stop an Inactive Logger!!!";
  }
}

bool sigmf_recorder_impl::start(Call *call) {
  if (state == INACTIVE) {
    timestamp = time(NULL);
    starttime = time(NULL);
    int nchars;
    tm *ltm = localtime(&starttime);

    talkgroup = call->get_talkgroup();
    freq = call->get_freq();

    BOOST_LOG_TRIVIAL(info) << "sigmf_recorder.cc: Starting Logger   \t[ " << rec_num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]";

    std::stringstream path_stream;

    // Found some good advice on Streams and Strings here: https://blog.sensecodons.com/2013/04/dont-let-stdstringstreamstrcstr-happen.html
    path_stream << call->get_capture_dir() << "/" << call->get_short_name() << "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;
    std::string path_string = path_stream.str();
    boost::filesystem::create_directories(path_string);

    nchars = snprintf(filename, 255, "%s/%ld-%ld_%.0f.raw", path_string.c_str(), talkgroup, starttime, call->get_freq());
    if (nchars >= 255) {
      BOOST_LOG_TRIVIAL(error) << "Call: Path longer than 255 charecters";
    }

    raw_sink->open(filename);
    state = ACTIVE;
    valve->set_enabled(true);
  } else {
    BOOST_LOG_TRIVIAL(error) << "sigmf_recorder.cc: Trying to Start an already Active Logger!!!";
  }
  return true;
}
