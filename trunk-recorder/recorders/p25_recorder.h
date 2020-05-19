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
#include <gnuradio/filter/firdes.h>

#include <gnuradio/filter/pfb_arb_resampler_ccf.h>
#include <gnuradio/filter/fft_filter_fff.h>

#include <gnuradio/analog/pwr_squelch_cc.h>
#include <gnuradio/analog/feedforward_agc_cc.h>
#include <gnuradio/analog/pll_freqdet_cf.h>
#include <gnuradio/digital/diff_phasor_cc.h>

#include <gnuradio/block.h>
#include <gnuradio/blocks/copy.h>
#include <gnuradio/blocks/short_to_float.h>

#if GNURADIO_VERSION < 0x030800
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/blocks/multiply_const_ss.h>
#include <gnuradio/filter/fir_filter_ccc.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/fir_filter_fff.h>
#else
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/blocks/multiply.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/filter/fir_filter_blk.h>
#endif

#include <gnuradio/blocks/complex_to_arg.h>

#include <op25_repeater/include/op25_repeater/fsk4_demod_ff.h>
#include <op25_repeater/fsk4_slicer_fb.h>
#include <op25_repeater/include/op25_repeater/p25_frame_assembler.h>
#include <op25_repeater/include/op25_repeater/rx_status.h>
#include <op25_repeater/gardner_costas_cc.h>
#include <op25_repeater/vocoder.h>

#include <gnuradio/msg_queue.h>
#include <gnuradio/message.h>
#include <gnuradio/blocks/head.h>
#include <gnuradio/blocks/file_sink.h>

#include "recorder.h"
#include "../config.h"
#include <gr_blocks/nonstop_wavfile_sink.h>


class Source;
class p25_recorder;
typedef boost::shared_ptr<p25_recorder>p25_recorder_sptr;
p25_recorder_sptr make_p25_recorder(Source * src);
#include "../source.h"

class p25_recorder : public gr::hier_block2, public Recorder {
  friend p25_recorder_sptr make_p25_recorder(Source * src);

protected:
  p25_recorder();
  p25_recorder(std::string type);
  virtual void initialize(Source *src, gr::blocks::nonstop_wavfile_sink::sptr wav_sink);

public:
  virtual ~p25_recorder();
  DecimSettings get_decim(long speed);
  void initialize_prefilter();
  void initialize_qpsk();
  void initialize_fsk4();
  void initialize_p25();
  void    tune_offset(double f);
  void    tune_freq(double f);
  virtual void    start(Call *call);
  virtual void    stop();
  void    clear();
  double  get_freq();
  int     get_num();
  void    set_tdma(bool phase2);
  void    switch_tdma(bool phase2);
  void    set_tdma_slot(int slot);
  void    generate_arb_taps();
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

protected:

  State state;
  time_t timestamp;
  time_t starttime;
  long   talkgroup;
  std::string short_name;
  Call *call;
  Config *config;
  Source *source;
  double chan_freq;
  double center_freq;
  bool   qpsk_mod;

  gr::op25_repeater::p25_frame_assembler::sptr op25_frame_assembler;
  gr::op25_repeater::gardner_costas_cc::sptr costas_clock;
  gr::blocks::nonstop_wavfile_sink::sptr wav_sink;
  gr::blocks::copy::sptr valve;
  //gr::blocks::multiply_const_ss::sptr levels;
  gr::blocks::multiply_const_ff::sptr levels;

private:


  double system_channel_rate;
  double arb_rate;
  double samples_per_symbol;
  double symbol_rate;
  double initial_rate;
  long decim;
  double resampled_rate;
  double squelch_db;
  int    silence_frames;
  int    tdma_slot;
  bool   d_phase2_tdma;
  bool   double_decim;
  long   if1;
  long   if2;
  long   input_rate;
  const int phase1_samples_per_symbol = 5;
  const int phase2_samples_per_symbol = 4;
  const double phase1_symbol_rate = 4800;
  const double phase2_symbol_rate = 6000;

  std::vector<float> arb_taps;
  std::vector<float> sym_taps;
  std::vector<float> baseband_noise_filter_taps;
  std::vector<gr_complex>	bandpass_filter_coeffs;
  std::vector<float> lowpass_filter_coeffs;
  std::vector<float> cutoff_filter_coeffs;


  gr::analog::sig_source_c::sptr lo;
  gr::analog::sig_source_c::sptr bfo;
  gr::blocks::multiply_cc::sptr  mixer;



  /* GR blocks */
  gr::filter::fft_filter_ccc::sptr bandpass_filter;
  gr::filter::fft_filter_ccf::sptr lowpass_filter;
  gr::filter::fft_filter_ccf::sptr cutoff_filter;


  gr::filter::fir_filter_fff::sptr sym_filter;
  
  gr::filter::fft_filter_fff::sptr noise_filter;

  gr::digital::diff_phasor_cc::sptr diffdec;


  gr::filter::pfb_arb_resampler_ccf::sptr arb_resampler;
  gr::blocks::short_to_float::sptr converter;
  gr::analog::feedforward_agc_cc::sptr   agc;
  gr::blocks::multiply_const_ff::sptr    pll_amp;
  gr::analog::pll_freqdet_cf::sptr       pll_freq_lock;
  gr::analog::pwr_squelch_cc::sptr       squelch;





  gr::blocks::multiply_const_ff::sptr rescale;

  gr::blocks::complex_to_arg::sptr    to_float;

  gr::op25_repeater::fsk4_demod_ff::sptr fsk4_demod;

  gr::op25_repeater::fsk4_slicer_fb::sptr slicer;

};


#endif // ifndef P25_RECORDER_H
