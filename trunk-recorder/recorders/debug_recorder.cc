
#include "debug_recorder.h"
#include <boost/log/trivial.hpp>

//static int rec_counter=0;
  
debug_recorder_sptr make_debug_recorder(Source *src)
{
  return gnuradio::get_initial_sptr(new debug_recorder(src));
}

debug_recorder::debug_recorder(Source *src)
  : gr::hier_block2("debug_recorder",
                    gr::io_signature::make(1, 1, sizeof(gr_complex)),
                    gr::io_signature::make(0, 0, sizeof(float))), Recorder("D")
{
  source = src;
  freq   = source->get_center();
  center = source->get_center();
  config = source->get_config();
  long samp_rate = source->get_rate();
  qpsk_mod  = source->get_qpsk_mod();
  silence_frames = source->get_silence_frames();
  talkgroup = 0;
  long capture_rate = samp_rate;


  rec_num = rec_counter++;

  state = inactive;

  double offset = freq - center;


  //double symbol_rate         = 4800;


  timestamp = time(NULL);
  starttime = time(NULL);




  double system_channel_rate = 96000;
  double xlate_bandwidth = 50000; // 24260.0


  valve = gr::blocks::copy::make(sizeof(gr_complex));
  valve->set_enabled(false);

  lpf_coeffs = gr::filter::firdes::low_pass(1.0, capture_rate, xlate_bandwidth/2, 3000, gr::filter::firdes::WIN_HANN);

  int decimation = floor(capture_rate / system_channel_rate);

  std::vector<gr_complex> dest(lpf_coeffs.begin(), lpf_coeffs.end());


     prefilter = gr::filter::freq_xlating_fir_filter_ccf::make(decimation,
                lpf_coeffs,
                offset,
                samp_rate);
/*
  prefilter = make_freq_xlating_fft_filter(decimation,
                                           dest,
                                           offset,
                                           samp_rate);
*/

  double resampled_rate = double(capture_rate) / double(decimation); // rate at
                                                                   // output of
                                                                   // self.lpf
  double arb_rate  = (double(system_channel_rate) / resampled_rate);
  double arb_size  = 32;
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
    double bw       = percent * halfband;
    double tb       = (percent / 2.0) * halfband;

    BOOST_LOG_TRIVIAL(info) << "Arb Rate: " << arb_rate << " Half band: " << halfband << " bw: " << bw << " tb: " << tb;

    // As we drop the bw factor, the optfir filter has a harder time converging;
    // using the firdes method here for better results.
    arb_taps = gr::filter::firdes::low_pass_2(arb_size, arb_size, bw, tb, arb_atten,
                                              gr::filter::firdes::WIN_BLACKMAN_HARRIS);
  } else {
    BOOST_LOG_TRIVIAL(error) << "Something is probably wrong! Resampling rate too low";
    exit(0);

    /*
       double halfband = 0.5;
       double bw = percent*halfband;
       double tb = (percent/2.0)*halfband;
       double ripple = 0.1;

       bool made = False;
       while not made:
        try:
            self._taps = optfir.low_pass(self._size, self._size, bw, bw+tb,
               ripple, atten)
            made = True
        except RuntimeError:
            ripple += 0.01
            made = False
            print("Warning: set ripple to %.4f dB. If this is a problem, adjust
               the attenuation or create your own filter taps." % (ripple))

     # Build in an exit strategy; if we've come this far, it ain't working.
            if(ripple >= 1.0):
                raise RuntimeError("optfir could not generate an appropriate
     #filter.")*/
  }


  arb_resampler = gr::filter::pfb_arb_resampler_ccf::make(arb_rate, arb_taps);


  //tm *ltm = localtime(&starttime);

  std::stringstream path_stream;
	//path_stream << boost::filesystem::current_path().string() <<  "/debug";
  path_stream << this->config->capture_dir << "/debug";

  boost::filesystem::create_directories(path_stream.str());
  int nchars = snprintf(filename, 160, "%s/%ld-%ld_%g.raw", path_stream.str().c_str(),talkgroup,starttime,freq);

  if (nchars >= 160) {
    BOOST_LOG_TRIVIAL(error) << "Analog Recorder: Path longer than 160 charecters";
  }
	raw_sink = gr::blocks::file_sink::make(sizeof(gr_complex), filename);



    connect(self(),               0, valve,                0);
    connect(valve,                0, prefilter,            0);
    connect(prefilter,            0, arb_resampler,        0);
        connect(arb_resampler,        0,  raw_sink,             0);

}



debug_recorder::~debug_recorder() {}


long debug_recorder::get_source_count() {
  return 0;
}

Call_Source * debug_recorder::get_source_list() {
  return NULL; //wav_sink->get_source_list();
}

Source * debug_recorder::get_source() {
  return source;
}

int debug_recorder::get_num() {
  return rec_num;
}

bool debug_recorder::is_active() {
  if (state == active) {
    return true;
  } else {
    return false;
  }
}

double debug_recorder::get_freq() {
  return freq;
}

double debug_recorder::get_current_length() {
  return 0; //wav_sink->length_in_seconds();
}

int debug_recorder::lastupdate() {
  return time(NULL) - timestamp;
}

long debug_recorder::elapsed() {
  return time(NULL) - starttime;
}

void debug_recorder::tune_offset(double f) {
  freq = f;
  int offset_amount = (f - center);
  prefilter->set_center_freq(offset_amount); // have to flip this for 3.7
  // BOOST_LOG_TRIVIAL(info) << "Offset set to: " << offset_amount << " Freq: "
  //  << freq;
}

State debug_recorder::get_state() {
  return state;
}

void debug_recorder::stop() {
  if (state == active) {
    recording_duration += wav_sink->length_in_seconds();
    BOOST_LOG_TRIVIAL(error) << "p25_recorder.cc: Stopping Logger \t[ " << rec_num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]";
    state = inactive;
    valve->set_enabled(false);
    raw_sink->close();
  } else {
    BOOST_LOG_TRIVIAL(error) << "p25_recorder.cc: Trying to Stop an Inactive Logger!!!";
  }
}

void debug_recorder::start(Call *call) {
  if (state == inactive) {
    timestamp = time(NULL);
    starttime = time(NULL);

    talkgroup = call->get_talkgroup();
    freq      = call->get_freq();


    BOOST_LOG_TRIVIAL(info) << "debug_recorder.cc: Starting Logger   \t[ " << rec_num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]";

    int offset_amount = (freq - center);
    prefilter->set_center_freq(offset_amount);

  	std::stringstream path_stream;

      //path_stream << boost::filesystem::current_path().string() <<  "/debug";
      path_stream << this->config->capture_dir << "/debug";

    boost::filesystem::create_directories(path_stream.str());
    sprintf(filename, "%s/%ld-%ld_%g.raw", path_stream.str().c_str(),talkgroup,starttime,freq);
	raw_sink->open(filename);
    state = active;
    valve->set_enabled(true);
  } else {
    BOOST_LOG_TRIVIAL(error) << "debug_recorder.cc: Trying to Start an already Active Logger!!!";
  }
}
