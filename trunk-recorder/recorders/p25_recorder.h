#ifndef P25_RECORDER_H
#define P25_RECORDER_H

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

#include <gnuradio/io_signature.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>
#include <gnuradio/filter/fft_filter_fff.h>

#include <gnuradio/analog/pwr_squelch_cc.h>
#include <gnuradio/analog/pwr_squelch_ff.h>
#include <gnuradio/analog/feedforward_agc_cc.h>
#include <gnuradio/analog/pll_freqdet_cf.h>
#include <gnuradio/digital/diff_phasor_cc.h>

#include <gnuradio/block.h>
#include <gnuradio/blocks/copy.h>
#include <gnuradio/blocks/short_to_float.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/blocks/complex_to_arg.h>

#include "../../op25_repeater/include/op25_repeater/fsk4_demod_ff.h"
#include "../../op25_repeater/include/op25_repeater/fsk4_slicer_fb.h"
#include "../../op25_repeater/include/op25_repeater/p25_frame_assembler.h"
#include "../../op25_repeater/include/op25_repeater/rx_status.h"
#include "../../op25_repeater/include/op25_repeater/gardner_costas_cc.h"
#include "../../op25_repeater/include/op25_repeater/vocoder.h"

#include <gnuradio/msg_queue.h>
#include <gnuradio/message.h>
#include <gnuradio/blocks/head.h>
#include <gnuradio/blocks/file_sink.h>

#include "recorder.h"
#include "../config.h"
#include "../../gr_blocks/nonstop_wavfile_sink.h"
#include "../../gr_blocks/freq_xlating_fft_filter.h"


class Source;
class p25_recorder;
typedef boost::shared_ptr<p25_recorder>p25_recorder_sptr;
p25_recorder_sptr make_p25_recorder(Source *src);
#include "../source.h"

class p25_recorder : public gr::hier_block2, public Recorder {
  friend p25_recorder_sptr make_p25_recorder(Source *src);

protected:

  p25_recorder(Source *src);

public:

  ~p25_recorder();

  void    tune_offset(double f);
  void    start(Call *call, int   n);
  void    stop();
  void    clear();
  double  get_freq();
  int     get_num();
  void    set_tdma_slot(int slot); 
  double  get_current_length();
  bool    is_active();
  bool    is_idle();
  State   get_state();
  Rx_Status get_rx_status();
  int     lastupdate();
  long    elapsed();
  Source* get_source();
  void    autotune();
  void    reset();
  gr::msg_queue::sptr tune_queue;
  gr::msg_queue::sptr traffic_queue;
  gr::msg_queue::sptr rx_queue;

private:

  double center_freq, chan_freq;
  double system_channel_rate;
  bool   qpsk_mod;
  double squelch_db;
  int    silence_frames;
  int    tdma_slot;
  long   talkgroup;
  bool phase2_tdma;
  std::string short_name;
  time_t timestamp;
  time_t starttime;

  Config *config;
  Source *source;
  char    filename[160];
  char    raw_filename[160];

  State state;

  std::vector<float> inital_lpf_taps;
  std::vector<float> channel_lpf_taps;
  std::vector<float> arb_taps;
  std::vector<float> sym_taps;
  std::vector<float> baseband_noise_filter_taps;

  freq_xlating_fft_filter_sptr prefilter;

  /* GR blocks */
  gr::filter::fft_filter_ccf::sptr channel_lpf;
  gr::filter::fir_filter_fff::sptr sym_filter;
  gr::filter::fft_filter_fff::sptr noise_filter;

  gr::digital::diff_phasor_cc::sptr diffdec;


  gr::filter::pfb_arb_resampler_ccf::sptr arb_resampler;

  gr::analog::feedforward_agc_cc::sptr   agc;
  gr::analog::pll_freqdet_cf::sptr       pll_freq_lock;
  gr::analog::pwr_squelch_cc::sptr       squelch;
  gr::analog::pwr_squelch_ff::sptr       squelch_two;
  gr::blocks::nonstop_wavfile_sink::sptr wav_sink;

  gr::blocks::short_to_float::sptr converter;
  gr::blocks::copy::sptr valve;

  gr::blocks::multiply_const_ff::sptr levels;
  gr::blocks::multiply_const_ff::sptr rescale;
  gr::blocks::multiply_const_ff::sptr baseband_amp;
  gr::blocks::multiply_const_ff::sptr pll_amp;
  gr::blocks::complex_to_arg::sptr    to_float;

  gr::op25_repeater::fsk4_demod_ff::sptr fsk4_demod;
  gr::op25_repeater::p25_frame_assembler::sptr op25_frame_assembler;
  gr::op25_repeater::fsk4_slicer_fb::sptr slicer;
  gr::op25_repeater::vocoder::sptr op25_vocoder;
  gr::op25_repeater::gardner_costas_cc::sptr costas_clock;

};


#endif // ifndef P25_RECORDER_H
