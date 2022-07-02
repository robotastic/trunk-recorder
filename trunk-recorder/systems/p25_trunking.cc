
#include "p25_trunking.h"
#include <boost/log/trivial.hpp>

p25_trunking_sptr make_p25_trunking(double freq, double center, long s, gr::msg_queue::sptr queue, bool qpsk, int sys_num) {
  return gnuradio::get_initial_sptr(new p25_trunking(freq, center, s, queue, qpsk, sys_num));
}

void p25_trunking::generate_arb_taps() {

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

p25_trunking::DecimSettings p25_trunking::get_decim(long speed) {
  long s = speed;
  long if_freqs[] = {24000, 25000, 32000};
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
    BOOST_LOG_TRIVIAL(debug) << "P25 trunking Decim: " << decim_settings.decim << " Decim2:  " << decim_settings.decim2;
    return decim_settings;
  }
  BOOST_LOG_TRIVIAL(error) << "p25 trunking Decim: Nothing found";
  return decim_settings;
}
void p25_trunking::initialize_prefilter() {
  double phase1_channel_rate = phase1_symbol_rate * phase1_samples_per_symbol;
  long if_rate = phase1_channel_rate;
  long fa = 0;
  long fb = 0;
  if1 = 0;
  if2 = 0;
  samples_per_symbol = phase1_samples_per_symbol;
  symbol_rate = phase1_symbol_rate;
  system_channel_rate = symbol_rate * samples_per_symbol;

  lo = gr::analog::sig_source_c::make(input_rate, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
  mixer = gr::blocks::multiply_cc::make();

  DecimSettings decim_settings = get_decim(input_rate);
  if (decim_settings.decim != -1) {
    double_decim = true;
    decim = decim_settings.decim;
    if1 = input_rate / decim_settings.decim;
    if2 = if1 / decim_settings.decim2;
    fa = 6250;
    fb = if2 / 2;
    BOOST_LOG_TRIVIAL(info) << "\t P25 Trunking two-stage decimator - Initial decimated rate: " << if1 << " Second decimated rate: " << if2 << " FA: " << fa << " FB: " << fb << " System Rate: " << input_rate;
    bandpass_filter_coeffs = gr::filter::firdes::complex_band_pass(1.0, input_rate, -if1 / 2, if1 / 2, if1 / 2);
    lowpass_filter_coeffs = gr::filter::firdes::low_pass(1.0, if1, (fb + fa) / 2, fb - fa);
    bandpass_filter = gr::filter::fft_filter_ccc::make(decim_settings.decim, bandpass_filter_coeffs);
    lowpass_filter = gr::filter::fft_filter_ccf::make(decim_settings.decim2, lowpass_filter_coeffs);
    resampled_rate = if2;
    bfo = gr::analog::sig_source_c::make(if1, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
  } else {
    double_decim = false;
    BOOST_LOG_TRIVIAL(info) << "\t P25 Trunking single-stage decimator - Initial decimated rate: " << if1 << " Second decimated rate: " << if2 << " Initial Decimation: " << decim << " System Rate: " << input_rate;
    lo = gr::analog::sig_source_c::make(input_rate, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
    lowpass_filter_coeffs = gr::filter::firdes::low_pass(1.0, input_rate, 7250, 1450);
    decim = floor(input_rate / if_rate);
    resampled_rate = input_rate / decim;

    lowpass_filter = gr::filter::fft_filter_ccf::make(decim, lowpass_filter_coeffs);
    resampled_rate = input_rate / decim;
  }

  // Cut-Off Filter
  fa = 6250;
  fb = fa + 625;
  cutoff_filter_coeffs = gr::filter::firdes::low_pass(1.0, if_rate, (fb + fa) / 2, fb - fa);
  cutoff_filter = gr::filter::fft_filter_ccf::make(1.0, cutoff_filter_coeffs);

  // ARB Resampler
  arb_rate = if_rate / resampled_rate;
  generate_arb_taps();
  arb_resampler = gr::filter::pfb_arb_resampler_ccf::make(arb_rate, arb_taps);
  BOOST_LOG_TRIVIAL(info) << "\t P25 Trunking ARB - Initial Rate: " << input_rate << " Resampled Rate: " << resampled_rate << " Initial Decimation: " << decim << " System Rate: " << system_channel_rate << " ARB Rate: " << arb_rate;

  double sps = phase1_samples_per_symbol;
  double def_excess_bw = 0.2;
  const double pi = M_PI;
  rms_agc = gr::blocks::rms_agc::make(0.45, 0.85);
  fll_band_edge = gr::digital::fll_band_edge_cc::make(sps, def_excess_bw, 2*sps+1, (2.0*pi)/sps/250); 

 
     

  if (double_decim) {
    connect(self(), 0, bandpass_filter, 0);
    connect(bandpass_filter, 0, mixer, 0);
    connect(bfo, 0, mixer, 1);
  } else {
    connect(self(), 0, mixer, 0);
    connect(lo, 0, mixer, 1);
  }
  connect(mixer, 0, lowpass_filter, 0);
  connect(lowpass_filter, 0, arb_resampler, 0);
  connect(arb_resampler, 0, cutoff_filter, 0);
  connect(cutoff_filter, 0, rms_agc, 0);
  connect(rms_agc,0, fll_band_edge, 0);
}

void p25_trunking::initialize_fsk4() {

  double phase1_channel_rate = phase1_symbol_rate * phase1_samples_per_symbol;
  // double phase2_channel_rate = phase2_symbol_rate * phase2_samples_per_symbol;
  const double pi = M_PI;

  // FSK4: Phase Loop Lock - can only be Phase 1, so locking at that rate.
  double freq_to_norm_radians = pi / (phase1_channel_rate / 2.0);
  double fc = 0.0;
  double fd = 600.0;
  double pll_demod_gain = 1.0 / (fd * freq_to_norm_radians);
  pll_freq_lock = gr::analog::pll_freqdet_cf::make((phase1_symbol_rate / 2.0 * 1.2) * freq_to_norm_radians, (fc + (3 * fd * 1.9)) * freq_to_norm_radians, (fc + (-3 * fd * 1.9)) * freq_to_norm_radians);
  pll_amp = gr::blocks::multiply_const_ff::make(pll_demod_gain * 1.0); // source->get_());

  // FSK4: noise filter - can only be Phase 1, so locking at that rate.
#if GNURADIO_VERSION < 0x030900
  baseband_noise_filter_taps = gr::filter::firdes::low_pass_2(1.0, phase1_channel_rate, phase1_symbol_rate / 2.0 * 1.175, phase1_symbol_rate / 2.0 * 0.125, 20.0, gr::filter::firdes::WIN_KAISER, 6.76);
#else
  baseband_noise_filter_taps = gr::filter::firdes::low_pass_2(1.0, phase1_channel_rate, phase1_symbol_rate / 2.0 * 1.175, phase1_symbol_rate / 2.0 * 0.125, 20.0, gr::fft::window::WIN_KAISER, 6.76);

#endif
  noise_filter = gr::filter::fft_filter_fff::make(1.0, baseband_noise_filter_taps);

  // FSK4: Symbol Taps
  double symbol_decim = 1;
  for (int i = 0; i < samples_per_symbol; i++) {
    sym_taps.push_back(1.0 / samples_per_symbol);
  }
  sym_filter = gr::filter::fir_filter_fff::make(symbol_decim, sym_taps);

  // FSK4: FSK4 Demod - locked at Phase 1 rates, since it can only be Phase 1
  tune_queue = gr::msg_queue::make(20);
  fsk4_demod = gr::op25_repeater::fsk4_demod_ff::make(tune_queue, phase1_channel_rate, phase1_symbol_rate);

  connect(fll_band_edge, 0, pll_freq_lock, 0);
  connect(pll_freq_lock, 0, pll_amp, 0);
  connect(pll_amp, 0, noise_filter, 0);
  connect(noise_filter, 0, sym_filter, 0);
  connect(sym_filter, 0, fsk4_demod, 0);
  connect(fsk4_demod, 0, slicer, 0);
}

void p25_trunking::initialize_qpsk() {
  const double pi = M_PI;

  agc = gr::analog::feedforward_agc_cc::make(16, 1.0);

  // Gardner Costas Clock
  double gain_mu = 0.025; // 0.025
  double costas_alpha = 0.04;
  double omega = double(system_channel_rate) / symbol_rate; // set to 6000 for TDMA, should be symbol_rate
  double gain_omega = 0.1 * gain_mu * gain_mu;
  double alpha = costas_alpha;
  double beta = 0.125 * alpha * alpha;
  double fmax = 3000; // Hz
  fmax = 2 * pi * fmax / double(system_channel_rate);

  //costas_clock = gr::op25_repeater::gardner_costas_cc::make(omega, gain_mu, gain_omega, alpha, beta, fmax, -fmax);
  costas = gr::op25_repeater::costas_loop_cc::make(costas_alpha,  4, (2 * pi)/4 ); //beta, fmax, -fmax);
  clock = gr::op25_repeater::gardner_cc::make(omega, gain_mu, gain_omega);


  // QPSK: Perform Differential decoding on the constellation
  diffdec = gr::digital::diff_phasor_cc::make();

  // QPSK: take angle of the difference (in radians)
  to_float = gr::blocks::complex_to_arg::make();

  // QPSK: convert from radians such that signal is in -3/-1/+1/+3
  rescale = gr::blocks::multiply_const_ff::make((1 / (pi / 4)));




  connect(fll_band_edge, 0, clock, 0);
  connect(clock, 0, diffdec, 0);
  connect(diffdec, 0, costas,0);
  connect(costas,0, to_float, 0);
  connect(to_float, 0, rescale, 0);
  connect(rescale, 0, slicer, 0);
}

void p25_trunking::initialize_p25() {
  // OP25 Slicer
  const float l[] = {-2.0, 0.0, 2.0, 4.0};
  const int msgq_id = 0;
  const int debug = 0;
  std::vector<float> slices(l, l + sizeof(l) / sizeof(l[0]));

  slicer = gr::op25_repeater::fsk4_slicer_fb::make(msgq_id, debug, slices);

  // OP25 Frame Assembler
  traffic_queue = gr::msg_queue::make(2);
  tune_queue = gr::msg_queue::make(2);

  int udp_port = 0;
  int verbosity = 0;
  const char *wireshark_host = "0";
  bool do_imbe = 0;
  int silence_frames = 0;
  bool do_output = 0;
  bool do_msgq = 1;
  bool do_audio_output = 0;
  bool do_tdma = 0;
  bool do_nocrypt = 1;
  op25_frame_assembler = gr::op25_repeater::p25_frame_assembler::make(sys_num, silence_frames, wireshark_host, udp_port, verbosity, do_imbe, do_output, do_msgq, rx_queue, do_audio_output, do_tdma, do_nocrypt);

  connect(slicer, 0, op25_frame_assembler, 0);
}

p25_trunking::p25_trunking(double f, double c, long s, gr::msg_queue::sptr queue, bool qpsk, int sys_num)
    : gr::hier_block2("p25_trunking",
                      gr::io_signature::make(1, 1, sizeof(gr_complex)),
                      gr::io_signature::make(0, 0, sizeof(float))) {

  this->sys_num = sys_num;
  chan_freq = f;
  center_freq = c;
  // long samp_rate = s;
  input_rate = s;
  rx_queue = queue;
  qpsk_mod = qpsk;

  initialize_prefilter();
  initialize_p25();

  if (!qpsk_mod) {
    initialize_fsk4();
  } else {
    initialize_qpsk();
  }
  tune_freq(chan_freq);
}

p25_trunking::~p25_trunking() {}

void p25_trunking::set_center(double c) {
  center_freq = c;
}

void p25_trunking::set_rate(long s) {
  input_rate = s;
  // TODO: Update/remake blocks that depend on input_rate
}

double p25_trunking::get_freq() {
  return chan_freq;
}

void p25_trunking::tune_freq(double f) {
  chan_freq = f;
  int offset_amount = (center_freq - f);
  tune_offset(offset_amount);
}

void p25_trunking::tune_offset(double f) {

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
  if (qpsk_mod) {
    costas->set_phase(0);
    costas->set_frequency(0);
  } else {
    fsk4_demod->reset();
  }
}
