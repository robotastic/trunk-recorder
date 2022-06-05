#ifndef SIGMF_RECORDER_IMPL_H
#define SIGMF_RECORDER_IMPL_H

#define _USE_MATH_DEFINES

#include <cstdio>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>

#include <gnuradio/filter/firdes.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/io_signature.h>

#include <gnuradio/analog/quadrature_demod_cf.h>

#include <gnuradio/analog/agc2_cc.h>
#include <gnuradio/analog/agc2_ff.h>
#include <gnuradio/analog/feedforward_agc_cc.h>
#include <gnuradio/digital/diff_phasor_cc.h>

#include <gnuradio/blocks/complex_to_arg.h>

#if GNURADIO_VERSION < 0x030800
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/blocks/multiply_const_cc.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/filter/freq_xlating_fir_filter_ccf.h>
#include <gnuradio/filter/rational_resampler_base_ccc.h>
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>
#else
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/blocks/multiply.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/filter/freq_xlating_fir_filter.h>
#if GNURADIO_VERSION < 0x030900
#include <gnuradio/filter/rational_resampler_base.h>
#else
#include <gnuradio/filter/rational_resampler.h>
#endif
#endif

#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>

#include <gnuradio/block.h>
#include <gnuradio/blocks/null_sink.h>

#include <gnuradio/blocks/copy.h>

#include <gnuradio/blocks/char_to_float.h>
#include <gnuradio/blocks/short_to_float.h>

#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/blocks/head.h>
#include <gnuradio/message.h>
#include <gnuradio/msg_queue.h>


#include "recorder.h"
#include "../gr_blocks/freq_xlating_fft_filter.h"

#include "../source.h"

class sigmf_recorder_impl : public  sigmf_recorder{

public:
  sigmf_recorder_impl(Source *src);

  void tune_offset(double f);
  bool start(Call *call);
  void stop();
  double get_freq();
  int get_num();
  double get_current_length();
  bool is_active();
  State get_state();
  int lastupdate();
  long elapsed();

  gr::msg_queue::sptr tune_queue;
  gr::msg_queue::sptr traffic_queue;
  gr::msg_queue::sptr rx_queue;
  //void forecast(int noutput_items, gr_vector_int &ninput_items_required);

private:
  double center, freq;
  int silence_frames;
  long talkgroup;
  time_t timestamp;
  time_t starttime;

  Config *config;
  Source *source;
  char filename[255];
  //int num;
  State state;

  //std::vector<gr_complex> lpf_coeffs;
  std::vector<float> lpf_coeffs;
  std::vector<float> arb_taps;
  std::vector<float> sym_taps;

  gr::filter::freq_xlating_fir_filter_ccf::sptr prefilter;



  gr::analog::quadrature_demod_cf::sptr fm_demod;
  gr::analog::feedforward_agc_cc::sptr agc;
  gr::analog::agc2_ff::sptr demod_agc;
  gr::analog::agc2_cc::sptr pre_demod_agc;
  gr::blocks::file_sink::sptr raw_sink;
  gr::blocks::short_to_float::sptr converter;
  gr::blocks::copy::sptr valve;


};

#endif
