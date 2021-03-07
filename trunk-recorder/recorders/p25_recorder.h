#ifndef P25_RECORDER_H
#define P25_RECORDER_H

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

#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>
#include <gnuradio/analog/pwr_squelch_cc.h>
#include <gnuradio/analog/pll_freqdet_cf.h>


#include <gnuradio/block.h>
#include <gnuradio/blocks/copy.h>

//#include <gnuradio/blocks/selector.h>

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


#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/blocks/head.h>
#include <gnuradio/message.h>
#include <gnuradio/msg_queue.h>

#include "../config.h"
#include "recorder.h"
#include "p25_recorder_decode.h"
#include "p25_recorder_fsk4_demod.h"
#include "p25_recorder_qpsk_demod.h"
#include <gr_blocks/nonstop_wavfile_sink.h>
#include <gr_blocks/selector.h>

class Source;
class p25_recorder;
typedef boost::shared_ptr<p25_recorder> p25_recorder_sptr;
p25_recorder_sptr make_p25_recorder(Source *src);
#include "../source.h"

class p25_recorder : public gr::hier_block2, public Recorder {
  friend p25_recorder_sptr make_p25_recorder(Source *src);

protected:
  p25_recorder();
  p25_recorder(std::string type);
  virtual void initialize(Source *src);

public:
  virtual ~p25_recorder();
  DecimSettings get_decim(long speed);
  void initialize_prefilter();
  void initialize_qpsk();
  void initialize_fsk4();
  void initialize_p25();
  void tune_offset(double f);
  void tune_freq(double f);
  virtual void start(Call *call);
  virtual void stop();
  void clear();
  double get_freq();
  int get_num();
  void set_tdma(bool phase2);
  void switch_tdma(bool phase2);
  void set_tdma_slot(int slot);
  void generate_arb_taps();
  double get_current_length();
  bool is_active();
  bool is_idle();
  State get_state();
  Rx_Status get_rx_status();
  char *get_filename();
  int lastupdate();
  long elapsed();
  Source *get_source();
  void autotune();
  void reset();

protected:
  State state;
  time_t timestamp;
  time_t starttime;
  long talkgroup;
  std::string short_name;
  Call *call;
  Config *config;
  Source *source;
  double chan_freq;
  double center_freq;
  bool qpsk_mod;
  bool conventional;
  double squelch_db;
  gr::analog::pwr_squelch_cc::sptr squelch;
  gr::blocks::selector::sptr modulation_selector;
  gr::blocks::copy::sptr valve;
  //gr::blocks::multiply_const_ss::sptr levels;


  p25_recorder_fsk4_demod_sptr fsk4_demod;
  p25_recorder_decode_sptr     fsk4_p25_decode;
  p25_recorder_qpsk_demod_sptr qpsk_demod;
  p25_recorder_decode_sptr     qpsk_p25_decode;

gr::op25_repeater::gardner_costas_cc::sptr costas_clock;
private:
  double system_channel_rate;
  double arb_rate;
  double samples_per_symbol;
  double symbol_rate;
  double initial_rate;
  long decim;
  double resampled_rate;

  int silence_frames;
  int tdma_slot;
  bool d_phase2_tdma;
  bool double_decim;
  long if1;
  long if2;
  long input_rate;
  const int phase1_samples_per_symbol = 5;
  const int phase2_samples_per_symbol = 4;
  const double phase1_symbol_rate = 4800;
  const double phase2_symbol_rate = 6000;

  std::vector<float> arb_taps;


 
  std::vector<gr_complex> bandpass_filter_coeffs;
  std::vector<float> lowpass_filter_coeffs;
  std::vector<float> cutoff_filter_coeffs;

  gr::analog::sig_source_c::sptr lo;
  gr::analog::sig_source_c::sptr bfo;
  gr::blocks::multiply_cc::sptr mixer;

  /* GR blocks */
  gr::filter::fft_filter_ccc::sptr bandpass_filter;
  gr::filter::fft_filter_ccf::sptr lowpass_filter;
  gr::filter::fft_filter_ccf::sptr cutoff_filter;

  gr::filter::pfb_arb_resampler_ccf::sptr arb_resampler;


  gr::blocks::multiply_const_ff::sptr rescale;



};

#endif // ifndef P25_RECORDER_H
