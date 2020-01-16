#ifndef SMARTNET_TRUNKING
#define SMARTNET_TRUNKING

#include <boost/math/constants/constants.hpp>
#include <boost/log/trivial.hpp>

#include <gnuradio/hier_block2.h>
#include <gnuradio/msg_queue.h>
#include <gnuradio/message.h>
#include <gnuradio/gr_complex.h>

#include <gnuradio/filter/firdes.h>

#if GNURADIO_VERSION < 0x030800
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/blocks/multiply_cc.h>
#else
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/blocks/multiply.h>
#endif

#include <gnuradio/filter/fft_filter_ccc.h>
#include <gnuradio/filter/fft_filter_ccf.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>
#include <gnuradio/digital/fll_band_edge_cc.h>
#include <gnuradio/digital/clock_recovery_mm_ff.h>
#include <gnuradio/digital/binary_slicer_fb.h>
#include <gnuradio/digital/correlate_access_code_tag_bb.h>


#include <gnuradio/analog/pll_freqdet_cf.h>


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
  struct DecimSettings
    {
		long decim;
		long decim2;
	};
  void set_center(double c);
  void set_rate(long s);
  void tune_offset(double f);
  void    tune_freq(double f);
  void reset();

protected:
  smartnet_trunking::DecimSettings get_decim(long speed);
  void    generate_arb_taps();
  void initialize_prefilter();


  std::vector<float> arb_taps;
  std::vector<float> baseband_noise_filter_taps;
  std::vector<gr_complex>	bandpass_filter_coeffs;
  std::vector<float> lowpass_filter_coeffs;
  std::vector<float> cutoff_filter_coeffs;

  gr::analog::sig_source_c::sptr lo;
  gr::analog::sig_source_c::sptr bfo;
  gr::blocks::multiply_cc::sptr  mixer;

  gr::filter::fft_filter_ccc::sptr bandpass_filter;
  gr::filter::fft_filter_ccf::sptr lowpass_filter;
  gr::filter::fft_filter_ccf::sptr cutoff_filter;

  gr::filter::pfb_arb_resampler_ccf::sptr arb_resampler;
  gr::digital::fll_band_edge_cc::sptr carriertrack;
  gr::analog::pll_freqdet_cf::sptr pll_demod;
  gr::digital::clock_recovery_mm_ff::sptr softbits;
  gr::digital::binary_slicer_fb::sptr slicer;
  gr::digital::correlate_access_code_tag_bb::sptr start_correlator;

  
  smartnet_trunking(float               f,
                    float               c,
                    long                s,
                    gr::msg_queue::sptr queue,
                    int                 sys_num);
  double chan_freq, center_freq;
  double system_channel_rate;
  double arb_rate;
  double samples_per_symbol;
  double symbol_rate;
  double resampled_rate;
  long input_rate;
  long decim;
  bool   double_decim;
  long   if1;
  long   if2;
  int    sys_num;

};


#endif // ifndef SMARTNET_TRUNKING
