#ifndef SMARTNET_TRUNKING
#define SMARTNET_TRUNKING

#include <boost/math/constants/constants.hpp>
#include <boost/log/trivial.hpp>

#include <gnuradio/hier_block2.h>
#include <gnuradio/msg_queue.h>
#include <gnuradio/message.h>
#include <gnuradio/gr_complex.h>

#include <gnuradio/filter/firdes.h>

#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>
#include <gnuradio/digital/fll_band_edge_cc.h>
#include <gnuradio/digital/clock_recovery_mm_ff.h>
#include <gnuradio/digital/binary_slicer_fb.h>
#include <gnuradio/digital/correlate_access_code_tag_bb.h>

#include <gnuradio/analog/pll_freqdet_cf.h>

#include <gr_blocks/freq_xlating_fft_filter.h>

#include "smartnet_decode.h"

class smartnet_trunking;

typedef boost::shared_ptr<smartnet_trunking>smartnet_trunking_sptr;

smartnet_trunking_sptr make_smartnet_trunking(float               f,
                                              float               c,
                                              long                s,
                                              gr::msg_queue::sptr queue,
                                              int                 sys_num);

class smartnet_trunking : public gr::hier_block2 {
  friend smartnet_trunking_sptr make_smartnet_trunking(float               f,
                                                       float               c,
                                                       long                s,
                                                       gr::msg_queue::sptr queue,
                                                       int                 sys_num);

public:
  void tune_offset(double f);
  void reset();

protected:

  std::vector<float> inital_lpf_taps;
  std::vector<float> channel_lpf_taps;
  std::vector<float> arb_taps;
  gr::filter::pfb_arb_resampler_ccf::sptr arb_resampler;
  gr::digital::fll_band_edge_cc::sptr carriertrack;
  gr::analog::pll_freqdet_cf::sptr pll_demod;
  gr::digital::clock_recovery_mm_ff::sptr softbits;
  gr::digital::binary_slicer_fb::sptr slicer;
  gr::digital::correlate_access_code_tag_bb::sptr start_correlator;

  freq_xlating_fft_filter_sptr prefilter;
  smartnet_trunking(float               f,
                    float               c,
                    long                s,
                    gr::msg_queue::sptr queue,
                    int                 sys_num);
  double samp_rate, chan_freq, center_freq;
  int    sys_num;

};


#endif // ifndef SMARTNET_TRUNKING
