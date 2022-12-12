
#include "debug_recorder_impl.h"
#include "debug_recorder.h"
#include <boost/log/trivial.hpp>
#if GNURADIO_VERSION >= 0x030a00
#include <gnuradio/network/udp_header_types.h>
#endif

// static int rec_counter=0;

debug_recorder_sptr make_debug_recorder(Source *src, std::string address, int port) {
  debug_recorder *recorder = new debug_recorder_impl(src, address, port);

  return gnuradio::get_initial_sptr(recorder);
}
void debug_recorder_impl::generate_arb_taps() {

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

debug_recorder_impl::DecimSettings debug_recorder_impl::get_decim(long speed) {
  long s = speed;
  long if_freqs[] = {32000};
  DecimSettings decim_settings = {-1, -1};
  for (int i = 0; i < 3; i++) {
    long if_freq = if_freqs[i];
    if (s % if_freq != 0) {
      continue;
    }
    long q = s / if_freq;
    if (q & 1) {
      continue;
    }

    if ((q >= 40) && ((q & 3) == 0)) {
      decim_settings.decim = q / 4;
      decim_settings.decim2 = 4;
    } else {
      decim_settings.decim = q / 2;
      decim_settings.decim2 = 2;
    }
    BOOST_LOG_TRIVIAL(debug) << "debug recorder Decim: " << decim_settings.decim << " Decim2:  " << decim_settings.decim2;
    return decim_settings;
  }
  BOOST_LOG_TRIVIAL(error) << "debug recorder Decim: Nothing found";
  return decim_settings;
}

void debug_recorder_impl::initialize_prefilter() {
  // double phase1_channel_rate = phase1_symbol_rate * phase1_samples_per_symbol;
  // double phase2_channel_rate = phase2_symbol_rate * phase2_samples_per_symbol;
  long if_rate = 32000;
  long fa = 0;
  long fb = 0;
  if1 = 0;
  if2 = 0;
  samples_per_symbol = phase1_samples_per_symbol;
  symbol_rate = phase1_symbol_rate;
  system_channel_rate = 32000; // symbol_rate * samples_per_symbol;

  valve = gr::blocks::copy::make(sizeof(gr_complex));
  valve->set_enabled(false);
  lo = gr::analog::sig_source_c::make(input_rate, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
  mixer = gr::blocks::multiply_cc::make();

  debug_recorder_impl::DecimSettings decim_settings = get_decim(input_rate);
  if (decim_settings.decim != -1) {
    double_decim = true;
    decim = decim_settings.decim;
    if1 = input_rate / decim_settings.decim;
    if2 = if1 / decim_settings.decim2;
    fa = 6250;
    fb = if2 / 2;
    BOOST_LOG_TRIVIAL(info) << "\t P25 Recorder two-stage decimator - Initial decimated rate: " << if1 << " Second decimated rate: " << if2 << " FA: " << fa << " FB: " << fb << " System Rate: " << input_rate;
    bandpass_filter_coeffs = gr::filter::firdes::complex_band_pass(1.0, input_rate, -if1 / 2, if1 / 2, if1 / 2);
    lowpass_filter_coeffs = gr::filter::firdes::low_pass(1.0, if1, (fb + fa) / 2, fb - fa);
    bandpass_filter = gr::filter::fft_filter_ccc::make(decim_settings.decim, bandpass_filter_coeffs);
    lowpass_filter = gr::filter::fft_filter_ccf::make(decim_settings.decim2, lowpass_filter_coeffs);
    resampled_rate = if2;
    bfo = gr::analog::sig_source_c::make(if1, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
  } else {
    double_decim = false;
    BOOST_LOG_TRIVIAL(info) << "\t P25 Recorder single-stage decimator - Initial decimated rate: " << if1 << " Second decimated rate: " << if2 << " Initial Decimation: " << decim << " System Rate: " << input_rate;
    lo = gr::analog::sig_source_c::make(input_rate, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
    lowpass_filter_coeffs = gr::filter::firdes::low_pass(1.0, input_rate, 12000, 2000);
    decim = floor(input_rate / if_rate);
    resampled_rate = input_rate / decim;

    lowpass_filter = gr::filter::fft_filter_ccf::make(decim, lowpass_filter_coeffs);
    resampled_rate = input_rate / decim;
  }

  // ARB Resampler
  arb_rate = if_rate / resampled_rate;
  generate_arb_taps();
  arb_resampler = gr::filter::pfb_arb_resampler_ccf::make(arb_rate, arb_taps);
  BOOST_LOG_TRIVIAL(info) << "\t P25 Recorder ARB - Initial Rate: " << input_rate << " Resampled Rate: " << resampled_rate << " Initial Decimation: " << decim << " System Rate: " << system_channel_rate << " ARB Rate: " << arb_rate;

  // Squelch DB
  // on a trunked network where you know you will have good signal, a carrier
  // power squelch works well. real FM receviers use a noise squelch, where
  // the received audio is high-passed above the cutoff and then fed to a
  // reverse squelch. If the power is then BELOW a threshold, open the squelch.

  connect(self(), 0, valve, 0);
  if (double_decim) {
    connect(valve, 0, bandpass_filter, 0);
    connect(bandpass_filter, 0, mixer, 0);
    connect(bfo, 0, mixer, 1);
  } else {
    connect(valve, 0, mixer, 0);
    connect(lo, 0, mixer, 1);
  }
  connect(mixer, 0, lowpass_filter, 0);
  connect(lowpass_filter, 0, arb_resampler, 0);
}

debug_recorder_impl::debug_recorder_impl(Source *src, std::string address, int port)
    : gr::hier_block2("debug_recorder",
                      gr::io_signature::make(1, 1, sizeof(gr_complex)),
                      gr::io_signature::make(0, 0, sizeof(float))),
      Recorder(DEBUG) {
  source = src;
  chan_freq = source->get_center();
  center_freq = source->get_center();
  config = source->get_config();
  input_rate = source->get_rate();
  talkgroup = 0;

  state = INACTIVE;

  timestamp = time(NULL);
  starttime = time(NULL);

  initialize_prefilter();
#if GNURADIO_VERSION < 0x030a00
  udp_sink = gr::blocks::udp_sink::make(sizeof(gr_complex), address, port);
#else
  udp_sink = gr::network::udp_sink::make(sizeof(gr_complex), 1, address, port, HEADERTYPE_NONE, 1472, true);
#endif
  connect(arb_resampler, 0, udp_sink, 0);
}

long debug_recorder_impl::get_source_count() {
  return 0;
}

Call_Source *debug_recorder_impl::get_source_list() {
  return NULL; // wav_sink->get_source_list();
}

Source *debug_recorder_impl::get_source() {
  return source;
}

int debug_recorder_impl::get_num() {
  return rec_num;
}

bool debug_recorder_impl::is_active() {
  if (state == ACTIVE) {
    return true;
  } else {
    return false;
  }
}

double debug_recorder_impl::get_freq() {
  return chan_freq;
}

double debug_recorder_impl::get_current_length() {
  return 0; // wav_sink->length_in_seconds();
}

int debug_recorder_impl::lastupdate() {
  return time(NULL) - timestamp;
}

long debug_recorder_impl::elapsed() {
  return time(NULL) - starttime;
}

void debug_recorder_impl::tune_freq(double f) {
  chan_freq = f;
  float freq = (center_freq - f);
  tune_offset(freq);
}
void debug_recorder_impl::tune_offset(double f) {

  float freq = static_cast<float>(f);

  if (abs(freq) > ((input_rate / 2) - (if1 / 2))) {
    BOOST_LOG_TRIVIAL(info) << "Tune Offset: Freq exceeds limit: " << abs(freq) << " compared to: " << ((input_rate / 2) - (if1 / 2));
  }
  if (double_decim) {
    bandpass_filter_coeffs = gr::filter::firdes::complex_band_pass(1.0, input_rate, -freq - if1 / 2, -freq + if1 / 2, if1 / 2);
    bandpass_filter->set_taps(bandpass_filter_coeffs);
    float bfz = (static_cast<float>(decim) * -freq) / (float)input_rate;
    bfz = bfz - static_cast<int>(bfz);
    if (bfz < -0.5) {
      bfz = bfz + 1.0;
    }
    if (bfz > 0.5) {
      bfz = bfz - 1.0;
    }
    bfo->set_frequency(-bfz * if1);

  } else {
    lo->set_frequency(freq);
  }
}

State debug_recorder_impl::get_state() {
  return state;
}

void debug_recorder_impl::stop() {
  if (state == ACTIVE) {
    BOOST_LOG_TRIVIAL(error) << "debug_recorder.cc: Stopping Logger \t[ " << rec_num << " ] - freq[ " << chan_freq << "] \t talkgroup[ " << talkgroup << " ]";
    state = INACTIVE;
    valve->set_enabled(false);
  } else {
    BOOST_LOG_TRIVIAL(error) << "debug_recorder.cc: Trying to Stop an Inactive Logger!!!";
  }
}

bool debug_recorder_impl::start(Call *call) {
  if (state == INACTIVE) {
    timestamp = time(NULL);
    starttime = time(NULL);

    talkgroup = call->get_talkgroup();
    chan_freq = call->get_freq();

    BOOST_LOG_TRIVIAL(info) << "debug_recorder.cc: Starting Logger   \t[ " << rec_num << " ] - freq[ " << chan_freq << "] \t talkgroup[ " << talkgroup << " ]";

    int offset_amount = (center_freq - chan_freq);
    tune_offset(offset_amount);

    state = ACTIVE;
    valve->set_enabled(true);
  } else {
    BOOST_LOG_TRIVIAL(error) << "debug_recorder.cc: Trying to Start an already Active Logger!!!";
    return false;
  }
  return true;
}
