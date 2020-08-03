#ifndef ANALOG_RECORDER_H
#define ANALOG_RECORDER_H

#include <cstdio>
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>

#include <gnuradio/hier_block2.h>
#include <gnuradio/io_signature.h>

#include <gnuradio/block.h>
#include <gnuradio/blocks/copy.h>
#if GNURADIO_VERSION < 0x030800
#include <gnuradio/filter/fir_filter_fff.h>

#include <gnuradio/blocks/multiply_const_ff.h>
#else
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/filter/fir_filter_blk.h>
#endif
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/iir_filter_ffd.h>

#include <gnuradio/analog/pwr_squelch_cc.h>
#include <gnuradio/analog/pwr_squelch_ff.h>
#include <gnuradio/analog/quadrature_demod_cf.h>

#include <gnuradio/blocks/float_to_short.h>

#include <gnuradio/filter/pfb_arb_resampler_ccf.h>

class Source;
class analog_recorder;

#include "../config.h"
#include "../lib/gr_blocks/decoder_wrapper.h"
#include "../systems/system.h"
#include "recorder.h"
#include <gr_blocks/decoder_wrapper.h>
#include <gr_blocks/freq_xlating_fft_filter.h>
#include <gr_blocks/nonstop_wavfile_sink.h>

typedef boost::shared_ptr<analog_recorder> analog_recorder_sptr;

#include "../source.h"

analog_recorder_sptr make_analog_recorder(Source *src);

class analog_recorder : public gr::hier_block2, public Recorder {
  friend analog_recorder_sptr make_analog_recorder(Source *src);

protected:
  analog_recorder(Source *src);

public:
  ~analog_recorder();
  void tune_offset(double f);
  void start(Call *call);
  void stop();
  double get_freq();
  Source *get_source();
  long get_talkgroup();
  time_t get_start_time();
  char *get_filename();
  double get_current_length();
  bool is_active();
  bool is_analog();
  bool is_idle();
  State get_state();
  int get_num();
  int lastupdate();
  long elapsed();
  static bool logging;

  void process_message_queues(void);
  void decoder_callback_handler(long unitId, const char *signaling_type, gr::blocks::SignalType signal);

private:
  double center_freq, chan_freq;
  long talkgroup;
  long samp_rate;

  double system_channel_rate;
  double squelch_db;
  time_t timestamp;
  time_t starttime;
  char filename[160];

  State state;
  std::vector<float> inital_lpf_taps;
  std::vector<float> channel_lpf_taps;
  std::vector<float> lpf_taps;
  std::vector<float> resampler_taps;
  std::vector<float> audio_resampler_taps;
  std::vector<float> sym_taps;
  std::vector<float> high_f_taps;
  std::vector<float> arb_taps;
  /* De-emph IIR filter taps */
  std::vector<double> d_fftaps; /*! Feed forward taps. */
  std::vector<double> d_fbtaps; /*! Feed back taps. */
  double d_tau;                 /*! De-emphasis time constant. */

  Call *call;
  Config *config;
  Source *source;
  void calculate_iir_taps(double tau);
  freq_xlating_fft_filter_sptr prefilter;

  /* GR blocks */
  gr::filter::iir_filter_ffd::sptr deemph;
  gr::filter::fir_filter_fff::sptr sym_filter;
  gr::filter::fft_filter_ccf::sptr channel_lpf;

  gr::blocks::multiply_const_ff::sptr levels;
  gr::filter::pfb_arb_resampler_ccf::sptr arb_resampler;
  gr::filter::fir_filter_fff::sptr decim_audio;
  gr::filter::fir_filter_fff::sptr high_f;
  gr::analog::pwr_squelch_cc::sptr squelch;
  gr::analog::pwr_squelch_ff::sptr squelch_two;
  gr::analog::quadrature_demod_cf::sptr demod;
  gr::blocks::float_to_short::sptr converter;

  gr::blocks::nonstop_wavfile_sink::sptr wav_sink;
  gr::blocks::copy::sptr valve;

  gr::blocks::decoder_wrapper::sptr decoder_sink;

  void setup_decoders_for_system(System *system);
};

#endif // ifndef ANALOG_RECORDER_H
