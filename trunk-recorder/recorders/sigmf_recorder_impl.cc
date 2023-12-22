
#include "sigmf_recorder_impl.h"
#include <boost/log/trivial.hpp>

// static int rec_counter=0;

void sigmf_recorder_impl::generate_arb_taps() {

  double arb_size = 32;
  double arb_atten = 100;
  // Create a filter that covers the full bandwidth of the output signal

  // If rate >= 1, we need to prevent images in the output,
  // so we have to filter it to less than half the channel
  // width of 0.5.  If rate < 1, we need to filter to less
  // than half the output signal's bw to avoid aliasing, so
  // the half-band here is 0.5*rate.
  double percent = 0.80;

  if (arb_rate <= 1) {
    double halfband = 0.5 * arb_rate;
    double bw = percent * halfband;
    double tb = (percent / 2.0) * halfband;

// BOOST_LOG_TRIVIAL(info) << "Arb Rate: " << arb_rate << " Half band: " << halfband << " bw: " << bw << " tb: " <<
// tb;

// As we drop the bw factor, the optfir filter has a harder time converging;
// using the firdes method here for better results.
#if GNURADIO_VERSION < 0x030900
    arb_taps = gr::filter::firdes::low_pass_2(arb_size, arb_size, bw, tb, arb_atten, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
#else
    arb_taps = gr::filter::firdes::low_pass_2(arb_size, arb_size, bw, tb, arb_atten, gr::fft::window::WIN_BLACKMAN_HARRIS);
#endif
  } else {
    BOOST_LOG_TRIVIAL(error) << "Something is probably wrong! Resampling rate too low";
    exit(1);
  }
}


void sigmf_recorder_impl::initialize_prefilter_xlat() {

  int decimation = int(input_rate / if_rate);
  double resampled_rate = float(input_rate) / float(decimation);

#if GNURADIO_VERSION < 0x030900
  if_coeffs = gr::filter::firdes::low_pass(1.0, input_rate, resampled_rate / 2, resampled_rate / 2, gr::filter::firdes::WIN_HAMMING);
#else
  if_coeffs = gr::filter::firdes::low_pass(1.0, input_rate, resampled_rate / 2, resampled_rate / 2, gr::fft::window::WIN_HAMMING);
#endif

  freq_xlat = gr::filter::freq_xlating_fir_filter<gr_complex, gr_complex, float>::make(decimation, if_coeffs, 0, input_rate); // inital_lpf_taps, 0, input_rate);
  connect(valve, 0, freq_xlat, 0);

  // ARB Resampler
  arb_rate = if_rate / resampled_rate;
  generate_arb_taps();
  arb_resampler = gr::filter::pfb_arb_resampler_ccf::make(arb_rate, arb_taps);
  connect(self(), 0, valve, 0);

  if (arb_rate == 1.0) {
    connect(freq_xlat, 0, squelch, 0);
  } else {
    connect(freq_xlat, 0, arb_resampler, 0);
    connect(arb_resampler, 0, squelch, 0);
  }

  /*connect(squelch, 0, rms_agc, 0);
  connect(rms_agc, 0, fll_band_edge, 0);*/
}


sigmf_recorder_sptr make_sigmf_recorder(Source *src) {
  sigmf_recorder *recorder = new sigmf_recorder_impl(src);

  return gnuradio::get_initial_sptr(recorder);
}

sigmf_recorder_impl::sigmf_recorder_impl(Source *src)
    : gr::hier_block2("sigmf_recorder",
                      gr::io_signature::make(1, 1, sizeof(gr_complex)),
                      gr::io_signature::make(0, 0, sizeof(float))),
      Recorder(SIGMF) {
  source = src;
  freq = source->get_center();
  center = source->get_center();
  config = source->get_config();
  silence_frames = source->get_silence_frames();
  squelch_db = 0;
  input_rate = source->get_rate();
  talkgroup = 0;
  recording_count = 0;
  recording_duration = 0;

  rec_num = rec_counter++;

  state = INACTIVE;

  // double symbol_rate         = 4800;

  timestamp = time(NULL);
  starttime = time(NULL);



  // tm *ltm = localtime(&starttime);

  int nchars = snprintf(filename, 160, "%ld-%ld_%g.raw", talkgroup, starttime, freq);

  if (nchars >= 160) {
    BOOST_LOG_TRIVIAL(error) << "Analog Recorder: Path longer than 160 charecters";
  }
  raw_sink = gr::blocks::file_sink::make(sizeof(gr_complex), filename);

  //initialize_prefilter();
  //initialize_prefilter_xlat();
  
  prefilter = channelizer::make(input_rate, channelizer::phase1_samples_per_symbol, channelizer::phase1_symbol_rate, center, true);
  prefilter->set_enabled(false);
  connect(squelch, 0, raw_sink, 0);
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
/*
void sigmf_recorder_impl::tune_offset(double f) {
  // have to flip this for 3.7
  // BOOST_LOG_TRIVIAL(info) << "Offset set to: " << offset_amount << " Freq: "
  //  << freq;
  freq_xlat->set_center_freq(-f);
}*/

State sigmf_recorder_impl::get_state() {
  return state;
}

void sigmf_recorder_impl::stop() {
  if (state == ACTIVE) {
    BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << this->call->get_talkgroup_display() << "\tFreq: " << format_freq(freq) << "\t\u001b[32mStopping SigMF Recorder Num [" << rec_num << "]\u001b[0m";

    state = INACTIVE;
    prefilter->set_enabled(false);
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
    this->call = call;
    System *system = call->get_system();
    talkgroup = call->get_talkgroup();
    freq = call->get_freq();
    squelch_db = system->get_squelch_db();
    prefilter->set_squelch_db(squelch_db);
    int offset_amount = (center - freq);
    prefilter->tune_offset(offset_amount);
    //freq_xlat->set_center_freq(-offset_amount);
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
    prefilter->set_enabled(true);
    std::string src_description = source->get_driver() + ": " + source->get_device() + " - " + source->get_antenna();
    time_t now;
    time(&now);
    char buf[sizeof "2011-10-08T07:07:09Z"];
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));
    std::string start_time(buf);
    nlohmann::json j = {
      {"global", {
        {"core:datatype", "cf32_le"},
        {"core:sample_rate", if_rate},
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
