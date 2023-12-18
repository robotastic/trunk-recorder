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

#include <gnuradio/digital/fll_band_edge_cc.h>
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
#include <json.hpp>
#include "../gr_blocks/rms_agc.h"
#include "recorder.h"

#include "../source.h"

class sigmf_recorder_impl : public sigmf_recorder {

public:
  sigmf_recorder_impl(Source * source, double freq, double rate, bool conventional);

  bool start(Call *call);
  bool start(System *sys, Config *config); 
   bool start();
  void stop();
  double get_freq();
  int get_num();
  long elapsed();
  void increment_active_cycles();
  long get_active_cycles();
  void increment_idle_cycles();
  long get_idle_cycles();
  void reset_idle_cycles();
  bool is_active();
  bool is_squelched();
  State get_state();



private:


  double center, freq;


      long input_rate;
  bool is_conventional;
  double squelch_db;
  time_t timestamp;
  time_t starttime;
  time_t lastupdate_time;
  long active_cycles;
  long idle_cycles;

  Config *config;
  Source *source;
  Call *call;
  long talkgroup;
  char filename[255];
  // int num;
  State state;

  gr::analog::pwr_squelch_cc::sptr squelch;
  gr::blocks::file_sink::sptr raw_sink;
  gr::blocks::copy::sptr valve;
};

#endif
