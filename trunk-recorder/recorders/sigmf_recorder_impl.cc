
#include "sigmf_recorder_impl.h"
#include <boost/log/trivial.hpp>




sigmf_recorder_sptr make_sigmf_recorder(Source *source, double freq, double rate, bool conventional) {
  sigmf_recorder *recorder = new sigmf_recorder_impl(source, freq, rate, conventional);

  return gnuradio::get_initial_sptr(recorder);
}

sigmf_recorder_impl::sigmf_recorder_impl(Source * source, double freq, double rate, bool conventional)
    : gr::hier_block2("sigmf_recorder",
                      gr::io_signature::make(1, 1, sizeof(gr_complex)),
                      gr::io_signature::make(0, 0, sizeof(float))),
      Recorder(SIGMF) {

  this->freq = freq; 
  center = 0;
  config = NULL;
  squelch_db = 0;
  this->input_rate = rate;
  this->source = source;
  is_conventional = conventional;
  active_cycles = 0;  
  idle_cycles = 0;
  rec_num = rec_counter++;
  state = INACTIVE;


  timestamp = time(NULL);
  starttime = time(NULL);



  // tm *ltm = localtime(&starttime);

  int nchars = snprintf(filename, 160, "%d-%ld_%g.raw", rec_num, starttime, freq);

  if (nchars >= 160) {
    BOOST_LOG_TRIVIAL(error) << "Analog Recorder: Path longer than 160 charecters";
  }
  squelch = gr::analog::pwr_squelch_cc::make(-160, 0.0001, 0, true);
  raw_sink = gr::blocks::file_sink::make(sizeof(gr_complex), filename);
  valve = gr::blocks::copy::make(sizeof(gr_complex));
  valve->set_enabled(false);

  if (is_conventional) {
    connect(self(), 0, squelch, 0);
    connect(squelch, 0, valve, 0);
  } else {
    connect(self(), 0, valve, 0);
  }
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

bool sigmf_recorder_impl::is_squelched() {
  return !squelch->unmuted();
}

double sigmf_recorder_impl::get_freq() {
  return freq;
}

void sigmf_recorder_impl::increment_active_cycles() {
  active_cycles++;
}

long sigmf_recorder_impl::get_active_cycles() {
  return active_cycles;
}

void sigmf_recorder_impl::increment_idle_cycles() {
  idle_cycles++;
}

long sigmf_recorder_impl::get_idle_cycles() {
  return idle_cycles;
}

void sigmf_recorder_impl::reset_idle_cycles() {
  idle_cycles = 0;
}


long sigmf_recorder_impl::elapsed() {
  return time(NULL) - starttime;
}

State sigmf_recorder_impl::get_state() {
  return state;
}

void sigmf_recorder_impl::stop() {
  if (state == ACTIVE) {
    //BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << this->call->get_talkgroup_display() << "\tFreq: " << format_freq(freq) << "\t\u001b[32mStopping SigMF Recorder Num [" << rec_num << "]\u001b[0m";

    state = INACTIVE;
    valve->set_enabled(false);
    raw_sink->close();
  } else {
    BOOST_LOG_TRIVIAL(error) << "sigmf_recorder.cc: Trying to Stop an Inactive Logger!!!";
  }
}


bool sigmf_recorder_impl::start(System *sys, Config *config) {
  if (state == INACTIVE) {
    timestamp = time(NULL);
    starttime = time(NULL);
    active_cycles = 0;  
    idle_cycles = 0;
    int nchars;
    tm *ltm = localtime(&starttime);
    System *system = sys;
    squelch_db = system->get_squelch_db();
    squelch->set_threshold(squelch_db);

    BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "] C\033[0m\tFreq: " << format_freq(freq) << "\t\u001b[32mStarting SigMF Recorder Num [" << rec_num << "]\u001b[0m";

    std::stringstream path_stream;

    // Found some good advice on Streams and Strings here: https://blog.sensecodons.com/2013/04/dont-let-stdstringstreamstrcstr-happen.html
    path_stream << config->temp_dir << "/" << sys->get_short_name() << "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;
    std::string path_string = path_stream.str();
    boost::filesystem::create_directories(path_string);

    nchars = snprintf(filename, 255, "%s/%d-%ld_%.0f.sigmf-data", path_string.c_str(), rec_num, starttime, freq);
    if (nchars >= 255) {
      BOOST_LOG_TRIVIAL(error) << "SigMF-meta: Path longer than 255 charecters";
    }

    raw_sink->open(filename);
    state = ACTIVE;
    valve->set_enabled(true);
    std::string src_description = source->get_driver() + ": " + source->get_device() + " - " + source->get_antenna();
    time_t now;
    time(&now);
    char buf[sizeof "2011-10-08T07:07:09Z"];
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));
    std::string start_time(buf);
    nlohmann::json j = {
      {"global", {
        {"core:datatype", "cf32_le"},
        {"core:sample_rate", input_rate},
        {"core:hw", src_description},
        {"core:recorder", "Trunk Recorder"},
        {"core:version", "1.0.0"}
      }},
      {"captures", nlohmann::json::array(
        { nlohmann::json::object({
          {"core:sample_start", 0},
          {"core:frequency", freq},
          {"core:datetime", start_time}
        })
        }
      )},
      {"annotations", nlohmann::json::array({})}
    };

    nchars = snprintf(filename, 255, "%s/%d-%ld_%.0f.sigmf-meta", path_string.c_str(), rec_num, starttime, freq);
    if (nchars >= 255) {
      BOOST_LOG_TRIVIAL(error) << "SigMF-meta: Path longer than 255 charecters";
    }
    std::ofstream o(filename);
    o << std::setw(4) << j << std::endl;
    o.close();

  } else {
    BOOST_LOG_TRIVIAL(error) << "sigmf_recorder.cc: Trying to Start an already Active Logger!!!";
  }
  return true;
}




bool sigmf_recorder_impl::start(Call *call) {
  if (state == INACTIVE) {
    timestamp = time(NULL);
    starttime = time(NULL);
    active_cycles = 0;  
    idle_cycles = 0;
    int nchars;
    tm *ltm = localtime(&starttime);
    this->call = call;
    System *system = call->get_system();
    talkgroup = call->get_talkgroup();
    freq = call->get_freq();
    squelch_db = system->get_squelch_db();
    squelch->set_threshold(squelch_db);
    int offset_amount = (center - freq);
    tune_offset(offset_amount);
    BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << this->call->get_talkgroup_display() << "\tFreq: " << format_freq(freq) << "\t\u001b[32mStarting SigMF Recorder Num [" << rec_num << "]\u001b[0m";

    std::stringstream path_stream;

    // Found some good advice on Streams and Strings here: https://blog.sensecodons.com/2013/04/dont-let-stdstringstreamstrcstr-happen.html
    path_stream << call->get_temp_dir() << "/" << call->get_short_name() << "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;
    std::string path_string = path_stream.str();
    boost::filesystem::create_directories(path_string);

    nchars = snprintf(filename, 255, "%s/%ld-%ld_%.0f-call_%lu.sigmf-data", path_string.c_str(), talkgroup, starttime, call->get_freq(), call->get_call_num());
    if (nchars >= 255) {
      BOOST_LOG_TRIVIAL(error) << "SigMF-meta: Path longer than 255 charecters";
    }

    raw_sink->open(filename);
    state = ACTIVE;
    valve->set_enabled(true);
    std::string src_description = source->get_driver() + ": " + source->get_device() + " - " + source->get_antenna();
    time_t now;
    time(&now);
    char buf[sizeof "2011-10-08T07:07:09Z"];
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));
    std::string start_time(buf);
    nlohmann::json j = {
      {"global", {
        {"core:datatype", "cf32_le"},
        {"core:sample_rate", input_rate},
        {"core:hw", src_description},
        {"core:recorder", "Trunk Recorder"},
        {"core:version", "1.0.0"}
      }},
      {"captures", nlohmann::json::array(
        { nlohmann::json::object({
          {"core:sample_start", 0},
          {"core:frequency", freq},
          {"core:datetime", start_time}
        })
        }
      )},
      {"annotations", nlohmann::json::array({})}
    };

    nchars = snprintf(filename, 255, "%s/%ld-%ld_%.0f-call_%lu.sigmf-meta", path_string.c_str(), talkgroup, starttime, call->get_freq(), call->get_call_num());
    if (nchars >= 255) {
      BOOST_LOG_TRIVIAL(error) << "SigMF-meta: Path longer than 255 charecters";
    }
    std::ofstream o(filename);
    o << std::setw(4) << j << std::endl;
    o.close();

  } else {
    BOOST_LOG_TRIVIAL(error) << "sigmf_recorder.cc: Trying to Start an already Active Logger!!!";
  }
  return true;
}
