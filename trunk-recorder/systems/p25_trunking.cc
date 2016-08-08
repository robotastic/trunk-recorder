
#include "p25_trunking.h"
#include <boost/log/trivial.hpp>


p25_trunking_sptr make_p25_trunking(double freq, double center, long s,  gr::msg_queue::sptr queue, bool qpsk, int sys_id)
{
  return gnuradio::get_initial_sptr(new p25_trunking(freq, center, s, queue, qpsk, sys_id));
}

p25_trunking::p25_trunking(double f, double c, long s, gr::msg_queue::sptr queue, bool qpsk, int sys_id)
  : gr::hier_block2("p25_trunking",
                    gr::io_signature::make(1, 1, sizeof(gr_complex)),
                    gr::io_signature::make(0, 0, sizeof(float)))
{
  this->sys_id = sys_id;
  freq         = f;
  center       = c;
  long samp_rate = s;

  float offset = freq - center;


  float  symbol_rate         = 4800;
  double samples_per_symbol  = 10;
  double system_channel_rate = symbol_rate * samples_per_symbol;
  float  symbol_deviation    = 600.0;
  qpsk_mod = qpsk;


  std::vector<float> sym_taps;
  const double pi = M_PI; // boost::math::constants::pi<double>();


  float if_rate      = 24000;
  float gain_mu      = 0.025;
  float costas_alpha = 0.04;
  float bb_gain      = 1.0;

  baseband_amp = gr::blocks::multiply_const_ff::make(bb_gain);


  float xlate_bandwidth = 14000; // 14000; //24260.0


  lpf_coeffs = gr::filter::firdes::low_pass(1.0, samp_rate, xlate_bandwidth / 2, 1500, gr::filter::firdes::WIN_HANN);
  int decimation = int(samp_rate / if_rate);

  std::vector<gr_complex> dest(lpf_coeffs.begin(), lpf_coeffs.end());

  prefilter = make_freq_xlating_fft_filter(decimation,
                                           dest,
                                           offset,
                                           samp_rate);

  /*
          prefilter = gr::filter::freq_xlating_fir_filter_ccf::make(decimation,
                      lpf_coeffs,
                      offset,
                      samp_rate);*/

  float resampled_rate = float(samp_rate) / float(decimation);

  // rate at output of self.lpf
  float arb_rate  = (float(if_rate) / resampled_rate);
  float arb_size  = 32;
  float arb_atten = 100;


  // Create a filter that covers the full bandwidth of the output signal

  // If rate >= 1, we need to prevent images in the output,
  // so we have to filter it to less than half the channel
  // width of 0.5.  If rate < 1, we need to filter to less
  // than half the output signal's bw to avoid aliasing, so
  // the half-band here is 0.5*rate.
  float percent = 0.80;

  if (arb_rate <= 1) {
    float halfband = 0.5 * arb_rate;
    float bw       = percent * halfband;
    float tb       = (percent / 2.0) * halfband;


    // As we drop the bw factor, the optfir filter has a harder time converging;
    // using the firdes method here for better results.
    arb_taps = gr::filter::firdes::low_pass_2(arb_size, arb_size, bw, tb, arb_atten,
                                              gr::filter::firdes::WIN_BLACKMAN_HARRIS);
  } else {
    BOOST_LOG_TRIVIAL(error) << "Something is probably wrong! Resampling rate too low";
    exit(0);
  }

  arb_resampler = gr::filter::pfb_arb_resampler_ccf::make(arb_rate, arb_taps);

  agc = gr::analog::feedforward_agc_cc::make(16, 1.0);

  float omega      = float(if_rate) / float(symbol_rate);
  float gain_omega = 0.1  * gain_mu * gain_mu;

  float alpha = costas_alpha;
  float beta  = 0.125 * alpha * alpha;
  float fmax  = 2400; // Hz
  fmax = 2 * pi * fmax / float(if_rate);

  costas_clock = gr::op25_repeater::gardner_costas_cc::make(omega, gain_mu, gain_omega, alpha,  beta, fmax, -fmax);


  // Perform Differential decoding on the constellation
  diffdec = gr::digital::diff_phasor_cc::make();

  // take angle of the difference (in radians)
  to_float = gr::blocks::complex_to_arg::make();

  // convert from radians such that signal is in -3/-1/+1/+3
  rescale = gr::blocks::multiply_const_ff::make((1 / (pi / 4)));

  // fm demodulator (needed in fsk4 case)
  float fm_demod_gain = if_rate / (2.0 * pi * symbol_deviation);
  fm_demod = gr::analog::quadrature_demod_cf::make(fm_demod_gain);

  double symbol_decim = 1;

  for (int i = 0; i < samples_per_symbol; i++) {
    sym_taps.push_back(1.0 / samples_per_symbol);
  }

  sym_filter    =  gr::filter::fir_filter_fff::make(symbol_decim, sym_taps);
  tune_queue    = gr::msg_queue::make(2);
  traffic_queue = gr::msg_queue::make(2);
  rx_queue      = queue;
  const float l[] = { -2.0, 0.0, 2.0, 4.0 };
  std::vector<float> levels(l, l + sizeof(l) / sizeof(l[0]));
  fsk4_demod = gr::op25_repeater::fsk4_demod_ff::make(tune_queue, system_channel_rate, symbol_rate);
  slicer     = gr::op25_repeater::fsk4_slicer_fb::make(levels);

  int udp_port               = 0;
  int verbosity              = 0;
  const char *wireshark_host = "127.0.0.1";
  bool do_imbe               = 0;
  bool do_output             = 0;
  bool do_msgq               = 1;
  bool do_audio_output       = 0;
  bool do_tdma               = 0;
  op25_frame_assembler = gr::op25_repeater::p25_frame_assembler::make(sys_id, wireshark_host, udp_port, verbosity, do_imbe, do_output, do_msgq, rx_queue, do_audio_output, do_tdma);


  /*
          to_float->set_max_output_buffer(4096);
          rescale->set_max_output_buffer(4096);
          slicer->set_max_output_buffer(4096);
          op25_frame_assembler->set_max_output_buffer(4096);
          //prefilter->set_max_output_buffer(512);
          arb_resampler->set_max_output_buffer(4096);
          fm_demod->set_max_output_buffer(4096);
          baseband_amp->set_max_output_buffer(4096);
          sym_filter->set_max_output_buffer(4096);
          fsk4_demod->set_max_output_buffer(4096);
          agc->set_max_output_buffer(4096);
          costas_clock->set_max_output_buffer(4096);
          diffdec->set_max_output_buffer(4096);*/

  // this->set_max_output_buffer(4096);


  if (!qpsk_mod) {
    connect(self(),        0, prefilter,            0);
    connect(prefilter,     0, arb_resampler,        0);
    connect(arb_resampler, 0, fm_demod,             0);
    connect(fm_demod,      0, baseband_amp,         0);
    connect(baseband_amp,  0, sym_filter,           0);
    connect(sym_filter,    0, fsk4_demod,           0);
    connect(fsk4_demod,    0, slicer,               0);
    connect(slicer,        0, op25_frame_assembler, 0);
  } else {
    connect(self(),        0, prefilter,            0);
    connect(prefilter,     0, arb_resampler,        0);
    connect(arb_resampler, 0, agc,                  0);
    connect(agc,           0, costas_clock,         0);
    connect(costas_clock,  0, diffdec,              0);
    connect(diffdec,       0, to_float,             0);
    connect(to_float,      0, rescale,              0);
    connect(rescale,       0, slicer,               0);
    connect(slicer,        0, op25_frame_assembler, 0);
  }
}

p25_trunking::~p25_trunking() {}

double p25_trunking::get_freq() {
  return freq;
}

void p25_trunking::tune_offset(double f) {
  freq = f;
  int offset_amount = (f - center);
  prefilter->set_center_freq(offset_amount); // have to flip this for 3.7
  // BOOST_LOG_TRIVIAL(info) << "Offset set to: " << offset_amount << " Freq: "
  //  << freq;
}
