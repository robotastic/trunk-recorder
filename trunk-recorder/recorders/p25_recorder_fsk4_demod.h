#ifndef P25_RECORDER_FSK4_DEMOD_H
#define P25_RECORDER_FSK4_DEMOD_H

#include <boost/shared_ptr.hpp>
#include <gnuradio/block.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/analog/pll_freqdet_cf.h>
#include <gnuradio/analog/pwr_squelch_cc.h>
#include <gnuradio/msg_queue.h>
#include <gnuradio/filter/fft_filter_fff.h>

#include <gnuradio/filter/fft_filter_ccf.h>



#if GNURADIO_VERSION < 0x030800
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/filter/fir_filter_fff.h>
#else
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/filter/fir_filter_blk.h>
#endif

#include <op25_repeater/fsk4_slicer_fb.h>
#include <op25_repeater/include/op25_repeater/fsk4_demod_ff.h>

class p25_recorder_fsk4_demod;
typedef boost::shared_ptr<p25_recorder_fsk4_demod> p25_recorder_fsk4_demod_sptr;
p25_recorder_fsk4_demod_sptr make_p25_recorder_fsk4_demod();

class p25_recorder_fsk4_demod : public gr::hier_block2 {
  friend p25_recorder_fsk4_demod_sptr make_p25_recorder_fsk4_demod();

protected:
  p25_recorder_fsk4_demod();
  virtual void initialize();

public:
  virtual ~p25_recorder_fsk4_demod();

private:

  const int phase1_samples_per_symbol = 5;
  const double phase1_symbol_rate = 4800;
    double squelch_db;
   std::vector<float> baseband_noise_filter_taps;
  std::vector<float> sym_taps;
    gr::msg_queue::sptr tune_queue;
 std::vector<float> cutoff_filter_coeffs;
  gr::filter::fft_filter_fff::sptr noise_filter;
  gr::filter::fir_filter_fff::sptr sym_filter;

  gr::filter::fft_filter_ccf::sptr cutoff_filter;
    gr::blocks::multiply_const_ff::sptr pll_amp;
  gr::analog::pll_freqdet_cf::sptr pll_freq_lock;
    gr::op25_repeater::fsk4_demod_ff::sptr fsk4_demod;
  gr::analog::pwr_squelch_cc::sptr squelch;
    gr::op25_repeater::fsk4_slicer_fb::sptr slicer;
};
#endif