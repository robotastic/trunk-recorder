#include "smartnet_trunking.h"

using namespace std;

smartnet_trunking_sptr make_smartnet_trunking(float               freq,
                                              float               center,
                                              long                samp,
                                              gr::msg_queue::sptr queue,
                                              int                 sys_id)
{
  return gnuradio::get_initial_sptr(new smartnet_trunking(freq, center, samp,
                                                          queue, sys_id));
}

smartnet_trunking::smartnet_trunking(float               f,
                                     float               c,
                                     long                s,
                                     gr::msg_queue::sptr queue,
                                     int                 sys_id)
  : gr::hier_block2("smartnet_trunking",
                    gr::io_signature::make(1, 1, sizeof(gr_complex)),
                    gr::io_signature::make(0, 0, sizeof(float)))
{
  center_freq  = c;
  chan_freq    = f;
  samp_rate    = s;
  this->sys_id = sys_id;
  float samples_per_second   = samp_rate;
  float syms_per_sec         = 3600;
  float gain_mu              = 0.01;
  float mu                   = 0.5;
  float omega_relative_limit = 0.3;
  float offset               = chan_freq - center_freq; // have to reverse it
                                                        // for 3.7 because it
                                                        // swapped in the
                                                        // switch.
  float clockrec_oversample = 3;
  int   decim               = int(samples_per_second / (syms_per_sec * clockrec_oversample));
  float sps                 = samples_per_second / decim / syms_per_sec;
  const double pi           = boost::math::constants::pi<double>();
  BOOST_LOG_TRIVIAL(info) <<  "SmartNet Trunking - SysId: " << sys_id;
  BOOST_LOG_TRIVIAL(info) <<  "Control channel offset: " << offset;
  BOOST_LOG_TRIVIAL(info) <<  "Control channel: " << chan_freq;
  BOOST_LOG_TRIVIAL(info) <<  "Decim: " << decim;
  BOOST_LOG_TRIVIAL(info) <<  "Samples per symbol: " << sps;

  std::vector<float> lpf_taps;


  lpf_taps = gr::filter::firdes::low_pass_2(1.0, samp_rate, 6250, 1500, 60,gr::filter::firdes::WIN_HANN);
  std::vector<gr_complex> dest(lpf_taps.begin(), lpf_taps.end());
  BOOST_LOG_TRIVIAL(info) << "Number of LPF taps: " << lpf_taps.size() << endl;

  /*prefilter = make_freq_xlating_fft_filter(decim,
                                                                         dest,
                                                                         offset,
                                                                         samp_rate);*/


  prefilter =
    gr::filter::freq_xlating_fir_filter_ccf::make(decim,
                                                  lpf_taps,
                                                  offset,
                                                  samp_rate);

  gr::digital::fll_band_edge_cc::sptr carriertrack =
    gr::digital::fll_band_edge_cc::make(sps, 0.6, 64, 0.35);

  gr::analog::pll_freqdet_cf::sptr pll_demod = gr::analog::pll_freqdet_cf::make(
    2.0 / clockrec_oversample,
    1 * pi / clockrec_oversample,
    -1 * pi / clockrec_oversample);

  gr::digital::clock_recovery_mm_ff::sptr softbits =
    gr::digital::clock_recovery_mm_ff::make(sps,
                                            0.25 * gain_mu * gain_mu,
                                            mu,
                                            gain_mu,
                                            omega_relative_limit);

  gr::digital::binary_slicer_fb::sptr slicer =
    gr::digital::binary_slicer_fb::make();

  gr::digital::correlate_access_code_tag_bb::sptr start_correlator =
    gr::digital::correlate_access_code_tag_bb::make("10101100",
                                                    0,
                                                    "smartnet_preamble");


  smartnet_decode_sptr decode = smartnet_make_decode(queue, sys_id);
  null_sink = gr::blocks::null_sink::make(sizeof(int8_t));
  connect(self(),           0, prefilter,        0);
  connect(prefilter,        0, carriertrack,     0);
  connect(carriertrack,     0, pll_demod,        0);
  connect(pll_demod,        0, softbits,         0);
  connect(softbits,         0, slicer,           0);
  connect(slicer,           0, start_correlator, 0);
  connect(start_correlator, 0, decode,           0);

}

void smartnet_trunking::tune_offset(double f) {
  chan_freq = f;
  int offset_amount = (f - center_freq);
  prefilter->set_center_freq(offset_amount); // have to flip this for 3.7
  cout << "Offset set to: " << offset_amount << " Freq: " << chan_freq << endl;
}
