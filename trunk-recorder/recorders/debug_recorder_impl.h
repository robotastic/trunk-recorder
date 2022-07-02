#ifndef DEBUG_RECORDER_IMPL_H
#define DEBUG_RECORDER_IMPL_H

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
#if GNURADIO_VERSION < 0x030a00
#include <gnuradio/blocks/udp_sink.h>
#else
#include <gnuradio/network/udp_sink.h>
#endif

#include <gnuradio/hier_block2.h>
#include <gnuradio/io_signature.h>

#include <gnuradio/analog/feedforward_agc_cc.h>
#include <gnuradio/analog/pll_freqdet_cf.h>
#include <gnuradio/analog/pwr_squelch_cc.h>
#include <gnuradio/digital/diff_phasor_cc.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>

#if GNURADIO_VERSION < 0x030800
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/blocks/multiply_const_ss.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/fir_filter_fff.h>
#else
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/blocks/multiply.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/filter/fir_filter_blk.h>
#endif

#include <gnuradio/block.h>
#include <gnuradio/blocks/complex_to_arg.h>
#include <gnuradio/blocks/copy.h>
#include <gnuradio/blocks/short_to_float.h>

#include <op25_repeater/fsk4_slicer_fb.h>
#include <op25_repeater/costas_loop_cc.h>
#include <op25_repeater/gardner_cc.h>
#include <op25_repeater/include/op25_repeater/fsk4_demod_ff.h>
#include <op25_repeater/include/op25_repeater/p25_frame_assembler.h>
#include <op25_repeater/include/op25_repeater/rx_status.h>
#include <op25_repeater/vocoder.h>

#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/blocks/head.h>
#include <gnuradio/message.h>
#include <gnuradio/msg_queue.h>

#include "../gr_blocks/freq_xlating_fft_filter.h"
#include "../source.h"
#include "debug_recorder.h"
#include "recorder.h"

class Source;
class debug_recorder;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<debug_recorder> debug_recorder_sptr;
#else
typedef std::shared_ptr<debug_recorder> debug_recorder_sptr;
#endif

class debug_recorder_impl : public debug_recorder {

public:
  debug_recorder_impl(Source *src, std::string address, int port);

  void tune_freq(double f);
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
  Source *get_source();
  long get_source_count();
  Call_Source *get_source_list();
  void initialize_prefilter();
  DecimSettings get_decim(long speed);
  void generate_arb_taps();
  // void forecast(int noutput_items, gr_vector_int &ninput_items_required);

private:
  double chan_freq;
  double center_freq;
  long talkgroup;
  time_t timestamp;
  time_t starttime;

  Config *config;
  Source *source;

  // int num;
  State state;

  double system_channel_rate;
  double arb_rate;
  double samples_per_symbol;
  double symbol_rate;

  long decim;
  double resampled_rate;
  bool double_decim;
  long if1;
  long if2;
  long input_rate;
  const int phase1_samples_per_symbol = 5;
  const double phase1_symbol_rate = 4800;

  std::vector<float> arb_taps;
  std::vector<float> sym_taps;
  std::vector<float> baseband_noise_filter_taps;
  std::vector<gr_complex> bandpass_filter_coeffs;
  std::vector<float> lowpass_filter_coeffs;
  std::vector<float> cutoff_filter_coeffs;

  gr::filter::fft_filter_ccc::sptr bandpass_filter;
  gr::filter::fft_filter_ccf::sptr lowpass_filter;
  gr::filter::fft_filter_ccf::sptr cutoff_filter;

  gr::blocks::copy::sptr valve;
  gr::analog::sig_source_c::sptr lo;
  gr::analog::sig_source_c::sptr bfo;
  gr::blocks::multiply_cc::sptr mixer;
#if GNURADIO_VERSION >= 0x030a00
  gr::network::udp_sink::sptr udp_sink;
#else
  gr::blocks::udp_sink::sptr udp_sink;
#endif
  gr::filter::pfb_arb_resampler_ccf::sptr arb_resampler;
};

#endif
