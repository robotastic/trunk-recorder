//
// SPDX-License-Identifier: GPL-3.0
//
// GNU Radio Python Flow Graph
// Title: RMS AGC
// Author: Daniel Estevez & Luke Berndt
// Description: AGC using RMS
// GNU Radio version: 3.8.0.0

#include "rms_agc.h"

namespace gr {
namespace blocks {

rms_agc::sptr rms_agc::make(double alpha, double reference) {
  return gnuradio::get_initial_sptr(
      new rms_agc(alpha, reference));
}

rms_agc::rms_agc(double a, double r) : gr::hier_block2("RMS AGC",
                                                       gr::io_signature::make(1, 1, sizeof(gr_complex)),
                                                       gr::io_signature::make(1, 1, sizeof(gr_complex))) {
  alpha = a;
  reference = r;
  rms_cf = gr::blocks::rms_cf::make(alpha);
  multiply_const = gr::blocks::multiply_const_ff::make(1.0 / reference);
  float_to_complex = gr::blocks::float_to_complex::make(1);
  divide = gr::blocks::divide_cc::make(1);
  add_const = gr::blocks::add_const_ff::make(1e-18);

  connect(add_const, 0, float_to_complex, 0);
  connect(divide, 0, self(), 0);
  connect(float_to_complex, 0, divide, 1);
  connect(multiply_const, 0, add_const, 0);
  connect(rms_cf, 0, multiply_const, 0);
  connect(self(), 0, divide, 0);
  connect(self(), 0, rms_cf, 0);
}

double rms_agc::get_alpha() {
  return alpha;
}

void rms_agc::set_alpha(double a) {
  alpha = a;
  rms_cf->set_alpha(alpha);
}

double rms_agc::get_reference() {
  return reference;
}

void rms_agc::set_reference(double r) {
  reference = r;
  multiply_const->set_k(1.0 / reference);
}

} // namespace blocks
} // namespace gr
