
#include "analog_recorder.h"
#include "../formatter.h"
#include "../../lib/gr_blocks/nonstop_wavfile_sink_impl.h"
using namespace std;

bool analog_recorder::logging = false;
//static int rec_counter=0;

analog_recorder_sptr make_analog_recorder(Source *src)
{
  return gnuradio::get_initial_sptr(new analog_recorder(src));
}

/*! \brief Calculate taps for FM de-emph IIR filter. */
void analog_recorder::calculate_iir_taps(double tau)
{
    // copied from fm_emph.py in gr-analog
    double  w_c;    // Digital corner frequency
    double  w_ca;   // Prewarped analog corner frequency
    double  k, z1, p1, b0;
    double  fs = system_channel_rate;

    w_c = 1.0 / tau;
    w_ca = 2.0 * fs * tan(w_c / (2.0 * fs));

    // Resulting digital pole, zero, and gain term from the bilinear
    // transformation of H(s) = w_ca / (s + w_ca) to
    // H(z) = b0 (1 - z1 z^-1)/(1 - p1 z^-1)
    k = -w_ca / (2.0 * fs);
    z1 = -1.0;
    p1 = (1.0 + k) / (1.0 - k);
    b0 = -k / (1.0 - k);

    d_fftaps[0] = b0;
    d_fftaps[1] = -z1 * b0;
    d_fbtaps[0] = 1.0;
    d_fbtaps[1] = -p1;
}

analog_recorder::analog_recorder(Source *src)
  : gr::hier_block2("analog_recorder",
                    gr::io_signature::make(1, 1, sizeof(gr_complex)),
                    gr::io_signature::make(0, 0, sizeof(float))), Recorder("A")
{
  //int nchars;

  source      = src;
  chan_freq   = source->get_center();
  center_freq = source->get_center();
  config      = source->get_config();
  samp_rate   = source->get_rate();
  talkgroup   = 0;

  rec_num = rec_counter++;
  state       = inactive;

  timestamp = time(NULL);
  starttime = time(NULL);

  float offset = 0;

  //int samp_per_sym        = 10;
  system_channel_rate     = 96000;//4800 * samp_per_sym;
/*  int decim               = floor(samp_rate / 384000);

  double pre_channel_rate = samp_rate / decim;*/

  int initial_decim      = floor(samp_rate / 480000);
  double initial_rate = double(samp_rate) / double(initial_decim);
  int decim = floor(initial_rate / system_channel_rate);
  double resampled_rate = double(initial_rate) / double(decim);

  inital_lpf_taps  = gr::filter::firdes::low_pass_2(1.0, samp_rate, 96000, 30000, 100, gr::filter::firdes::WIN_HANN);
//  channel_lpf_taps =  gr::filter::firdes::low_pass_2(1.0, pre_channel_rate, 5000, 2000, 60);
  channel_lpf_taps =  gr::filter::firdes::low_pass_2(1.0, initial_rate, 4000, 1000, 100);


  std::vector<gr_complex> dest(inital_lpf_taps.begin(), inital_lpf_taps.end());

  prefilter = make_freq_xlating_fft_filter(initial_decim, dest, offset, samp_rate);

  channel_lpf =  gr::filter::fft_filter_ccf::make(decim, channel_lpf_taps);

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

    // BOOST_LOG_TRIVIAL(info) << "Arb Rate: " << arb_rate << " Half band: " << halfband << " bw: " << bw << " tb: " <<
    // tb;

    // As we drop the bw factor, the optfir filter has a harder time converging;
    // using the firdes method here for better results.
    arb_taps = gr::filter::firdes::low_pass_2(arb_size, arb_size, bw, tb, arb_atten,
                                              gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    double tap_total = inital_lpf_taps.size() + channel_lpf_taps.size() + arb_taps.size();
    BOOST_LOG_TRIVIAL(info) << "P25 Recorder Taps - initial: " << inital_lpf_taps.size() << " channel: " << channel_lpf_taps.size() << " ARB: " << arb_taps.size() << " Total: " << tap_total;
  } else {
    BOOST_LOG_TRIVIAL(error) << "Something is probably wrong! Resampling rate too low";
    exit(0);
  }


  arb_resampler = gr::filter::pfb_arb_resampler_ccf::make(arb_rate, arb_taps);

  // on a trunked network where you know you will have good signal, a carrier
  // power squelch works well. real FM receviers use a noise squelch, where
  // the received audio is high-passed above the cutoff and then fed to a
  // reverse squelch. If the power is then BELOW a threshold, open the squelch.

  squelch_db = source->get_squelch_db();

  if (squelch_db != 0) {
    // Non-blocking as we are using squelch_two as a gate.
    squelch = gr::analog::pwr_squelch_cc::make(squelch_db, 0.01, 10, false);

    //  based on squelch code form ham2mon
    // set low -200 since its after demod and its just gate for previous squelch so that the audio
    // recording doesn't contain blank spaces between transmissions
    squelch_two = gr::analog::pwr_squelch_ff::make(-200, 0.01, 0, true);
  }


  // k = quad_rate/(2*math.pi*max_dev) = 48k / (6.283185*5000) = 1.527
  float quad_gain;
  int d_max_dev = 5000;
    /* demodulator gain */
    quad_gain = system_channel_rate / (2.0 * M_PI * d_max_dev);
  demod    = gr::analog::quadrature_demod_cf::make(quad_gain);
  levels   = gr::blocks::multiply_const_ff::make(src->get_analog_levels()); // 33);
  valve    = gr::blocks::copy::make(sizeof(gr_complex));
  valve->set_enabled(false);

  /* de-emphasis */
    d_tau  = 0.000075; // 75us
  d_fftaps.resize(2);
  d_fbtaps.resize(2);
  calculate_iir_taps(d_tau);
  deemph = gr::filter::iir_filter_ffd::make(d_fftaps, d_fbtaps);


  audio_resampler_taps = design_filter(1, 12);

  // downsample from 48k to 8k
  decim_audio = gr::filter::fir_filter_fff::make(12, audio_resampler_taps);

  //tm *ltm = localtime(&starttime);

  wav_sink = gr::blocks::nonstop_wavfile_sink_impl::make(1, 8000, 16, true);

  // Try and get rid of the FSK wobble
  high_f_taps =  gr::filter::firdes::high_pass(1, 8000, 300, 50, gr::filter::firdes::WIN_HANN);
  high_f      = gr::filter::fir_filter_fff::make(1, high_f_taps);


  if (squelch_db != 0) {
    // using squelch
    connect(self(),        0, valve,         0);
    connect(valve,         0, prefilter,     0);
    connect(prefilter,     0, channel_lpf,   0);
    connect(channel_lpf,   0, arb_resampler, 0);
    connect(arb_resampler, 0, squelch,       0);
    connect(squelch,       0, demod,         0);
    connect(demod,         0, deemph,        0);
    connect(deemph,        0, decim_audio,   0);
    connect(decim_audio,   0, high_f,        0);
    connect(high_f,        0, squelch_two,   0);
    connect(squelch_two,   0, levels,        0);
    connect(levels,        0, wav_sink,      0);
  } else {
    // No squelch used
    connect(self(),        0, valve,         0);
    connect(valve,         0, prefilter,     0);
    connect(prefilter,     0, channel_lpf,   0);
    connect(channel_lpf,   0, arb_resampler, 0);
    connect(arb_resampler, 0, demod,         0);
    connect(demod,         0, deemph,        0);
    connect(deemph,        0, decim_audio,   0);
    connect(decim_audio,   0, levels,        0);
    connect(levels,        0, wav_sink,      0);
  }
}

analog_recorder::~analog_recorder() {}

State analog_recorder::get_state() {
  return state;
}
int analog_recorder::get_num() {
  return rec_num;
}

void analog_recorder::stop() {
  if (state == active) {
    recording_duration += wav_sink->length_in_seconds();
    state = inactive;
    valve->set_enabled(false);
    wav_sink->close();
  } else {

    BOOST_LOG_TRIVIAL(error) << "analog_recorder.cc: Stopping an inactive Logger \t[ " << rec_num << " ] - freq[ " << FormatFreq(chan_freq) << "] \t talkgroup[ " << talkgroup << " ]";
  }
}

bool analog_recorder::is_analog() {
  return true;
}

bool analog_recorder::is_active() {
  if (state == active) {
    return true;
  } else {
    return false;
  }
}

bool analog_recorder::is_idle() {
  if (state == active) {
    return !squelch->unmuted();
  }
  return true;
}

long analog_recorder::get_talkgroup() {
  return talkgroup;
}

double analog_recorder::get_freq() {
  return chan_freq;
}

Source * analog_recorder::get_source() {
  return source;
}

int analog_recorder::lastupdate() {
  return time(NULL) - timestamp;
}

long analog_recorder::elapsed() {
  return time(NULL) - starttime;
}

time_t analog_recorder::get_start_time() {
  return starttime;
}

char * analog_recorder::get_filename() {
  return filename;
}

double analog_recorder::get_current_length() {
  return wav_sink->length_in_seconds();
}

void analog_recorder::tune_offset(double f) {
  chan_freq = f;
  int offset_amount = (f - center_freq);
  prefilter->set_center_freq(offset_amount);
}

void analog_recorder::start(Call *call) {
  starttime = time(NULL);

  talkgroup = call->get_talkgroup();
  chan_freq = call->get_freq();

  prefilter->set_center_freq(chan_freq - center_freq);

  wav_sink->open(call->get_filename());

  state = active;
  valve->set_enabled(true);
}
