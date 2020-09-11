#include "p25_recorder_qpsk_demod.h"

p25_recorder_qpsk_demod_sptr make_p25_recorder_qpsk_demod() {
  p25_recorder_qpsk_demod *recorder = new p25_recorder_qpsk_demod();
  recorder->initialize();
  return gnuradio::get_initial_sptr(recorder);
}

p25_recorder_qpsk_demod::p25_recorder_qpsk_demod()
    : gr::hier_block2("p25_recorder_qpsk_demod",
                      gr::io_signature::make(1, 1, sizeof(gr_complex)),
                      gr::io_signature::make(1, 1, sizeof(char))) {
}

p25_recorder_qpsk_demod::~p25_recorder_qpsk_demod() {
  
}

void p25_recorder_qpsk_demod::initialize() {
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

  costas_clock = gr::op25_repeater::gardner_costas_cc::make(omega, gain_mu, gain_omega, alpha, beta, fmax, -fmax);

  // QPSK: Perform Differential decoding on the constellation
  diffdec = gr::digital::diff_phasor_cc::make();

  // QPSK: take angle of the difference (in radians)
  to_float = gr::blocks::complex_to_arg::make();

  // QPSK: convert from radians such that signal is in -3/-1/+1/+3
  rescale = gr::blocks::multiply_const_ff::make((1 / (pi / 4)));


    connect(self(), 0, agc, 0);
  
  connect(agc, 0, costas_clock, 0);
  connect(costas_clock, 0, diffdec, 0);
  connect(diffdec, 0, to_float, 0);
  connect(to_float, 0, rescale, 0);
  connect(rescale, 0, self(), 0);
}
