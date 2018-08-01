#ifndef P25_TRUNKING_H
#define P25_TRUNKING_H

#define _USE_MATH_DEFINES

#include <cstdio>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <time.h>


#include <boost/shared_ptr.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>

#include <gnuradio/io_signature.h>
#include <gnuradio/hier_block2.h>

#include <gnuradio/block.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/blocks/complex_to_arg.h>
#include <gnuradio/blocks/short_to_float.h>

#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>

#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/analog/feedforward_agc_cc.h>

#include <gnuradio/digital/diff_phasor_cc.h>

#include <op25_repeater/fsk4_demod_ff.h>
#include <op25_repeater/fsk4_slicer_fb.h>
#include <op25_repeater/include/op25_repeater/p25_frame_assembler.h>
#include <op25_repeater/gardner_costas_cc.h>

#include <gnuradio/msg_queue.h>
#include <gnuradio/message.h>

#include <gr_blocks/freq_xlating_fft_filter.h>


class p25_trunking;

typedef boost::shared_ptr<p25_trunking>p25_trunking_sptr;

p25_trunking_sptr make_p25_trunking(double              f,
                                    double              c,
                                    long                s,
                                    gr::msg_queue::sptr queue,
                                    bool                qpsk,
                                    int                 sys_num);


class p25_trunking : public gr::hier_block2 {
  friend p25_trunking_sptr make_p25_trunking(double              f,
                                             double              c,
                                             long                s,
                                             gr::msg_queue::sptr queue,
                                             bool                qpsk,
                                             int                 sys_num);

protected:
  p25_trunking(double              f,
               double              c,
               long                s,
               gr::msg_queue::sptr queue,
               bool                qpsk,
               int                 sys_num);

public:

  ~p25_trunking();

  void   tune_offset(double f);
  double get_freq();
  void   enable();


  gr::msg_queue::sptr tune_queue;
  gr::msg_queue::sptr traffic_queue;
  gr::msg_queue::sptr rx_queue;


private:

  double center_freq, chan_freq;
  bool   qpsk_mod;
  int    sys_num;

  std::vector<float> inital_lpf_taps;
  std::vector<float> channel_lpf_taps;
  std::vector<float> arb_taps;
  std::vector<float> sym_taps;

  freq_xlating_fft_filter_sptr prefilter;

  /* GR blocks */
  gr::filter::fft_filter_ccf::sptr channel_lpf;
  gr::filter::fir_filter_fff::sptr sym_filter;
  gr::filter::pfb_arb_resampler_ccf::sptr arb_resampler;

  gr::digital::diff_phasor_cc::sptr diffdec;

  gr::analog::quadrature_demod_cf::sptr fm_demod;
  gr::analog::feedforward_agc_cc::sptr  agc;

  gr::blocks::short_to_float::sptr converter;
  gr::blocks::multiply_const_ff::sptr rescale;
  gr::blocks::multiply_const_ff::sptr baseband_amp;
  gr::blocks::complex_to_arg::sptr    to_float;

  gr::op25_repeater::fsk4_demod_ff::sptr fsk4_demod;
  gr::op25_repeater::p25_frame_assembler::sptr op25_frame_assembler;
  gr::op25_repeater::fsk4_slicer_fb::sptr slicer;
  gr::op25_repeater::gardner_costas_cc::sptr costas_clock;
};


#endif // ifndef P25_TRUNKING_H
