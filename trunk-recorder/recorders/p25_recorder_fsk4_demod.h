#ifndef P25_RECORDER_FSK4_DEMOD_H
#define P25_RECORDER_FSK4_DEMOD_H

#include <boost/shared_ptr.hpp>
#include <gnuradio/analog/pll_freqdet_cf.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/block.h>
#include <gnuradio/block_detail.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/msg_queue.h>

#include <gnuradio/filter/fft_filter_ccf.h>

#if GNURADIO_VERSION < 0x030800
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/filter/fir_filter_fff.h>
#else
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/filter/fir_filter_blk.h>
#endif
#include "../gr_blocks/rms_agc.h"
#include <op25_repeater/fsk4_slicer_fb.h>
#include <op25_repeater/rmsagc_ff.h>
#include <op25_repeater/include/op25_repeater/fsk4_demod_ff.h>
#include <gnuradio/digital/clock_recovery_mm_ff.h>

class p25_recorder_fsk4_demod;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<p25_recorder_fsk4_demod> p25_recorder_fsk4_demod_sptr;
#else
typedef std::shared_ptr<p25_recorder_fsk4_demod> p25_recorder_fsk4_demod_sptr;
#endif

p25_recorder_fsk4_demod_sptr make_p25_recorder_fsk4_demod();

class p25_recorder_fsk4_demod : public gr::hier_block2 {
  friend p25_recorder_fsk4_demod_sptr make_p25_recorder_fsk4_demod();

protected:
  virtual void initialize();

public:
  p25_recorder_fsk4_demod();
  virtual ~p25_recorder_fsk4_demod();
  void reset();

private:
  const int phase1_samples_per_symbol = 5;
  const double phase1_symbol_rate = 4800;
  std::vector<float> baseband_noise_filter_taps;
  std::vector<float> sym_taps;
  gr::msg_queue::sptr tune_queue;
  std::vector<float> cutoff_filter_coeffs;
  gr::filter::fft_filter_fff::sptr noise_filter;
  gr::filter::fir_filter_fff::sptr sym_filter;
  gr::filter::fft_filter_ccf::sptr cutoff_filter;
  gr::blocks::multiply_const_ff::sptr pll_amp;
  gr::analog::pll_freqdet_cf::sptr pll_freq_lock;
  gr::analog::quadrature_demod_cf::sptr fm_demod;
  gr::op25_repeater::rmsagc_ff::sptr baseband_amp;
  gr::op25_repeater::fsk4_demod_ff::sptr fsk4_demod;
  gr::op25_repeater::fsk4_slicer_fb::sptr slicer;
  gr::digital::clock_recovery_mm_ff::sptr clock_recovery;
    void reset_block(gr::basic_block_sptr block); 
};
#endif