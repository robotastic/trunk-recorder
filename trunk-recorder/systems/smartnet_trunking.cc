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

  double symbol_rate         = 3600;
  double samples_per_symbol  = 10; // was 10
  double system_channel_rate = symbol_rate * samples_per_symbol;
  int    decimation          = int(samp_rate / 96000);
  double resampled_rate      = double(samp_rate) / double(decimation);


  float gain_mu              = 0.01;
  float mu                   = 0.5;
  float omega_relative_limit = 0.3;
  float offset               = chan_freq - center_freq;

  const double pi = boost::math::constants::pi<double>();
  BOOST_LOG_TRIVIAL(info) <<  "SmartNet Trunking - SysId: " << sys_id;
  BOOST_LOG_TRIVIAL(info) <<  "Control channel: " << chan_freq;


  inital_lpf_taps  = gr::filter::firdes::low_pass_2(1.0, samp_rate, 96000, 25000, 60, gr::filter::firdes::WIN_HANN);
  channel_lpf_taps = gr::filter::firdes::low_pass_2(1.0, resampled_rate, 6250, 1500, 60, gr::filter::firdes::WIN_HANN);
  std::vector<gr_complex> dest(inital_lpf_taps.begin(), inital_lpf_taps.end());

  prefilter = make_freq_xlating_fft_filter(decimation, dest, offset, samp_rate);

  gr::filter::fft_filter_ccf::sptr channel_lpf = gr::filter::fft_filter_ccf::make(1.0, channel_lpf_taps);

  double arb_rate  = (double(system_channel_rate) / resampled_rate);
  double arb_size  = 32;
  double arb_atten = 100;


  // Create a filter that covers the full bandwidth of the output signal

  // If rate >= 1, we need to prevent images in the output,
  // so we have to filter it to less than half the channel
  // width of 0.5.  If rate < 1, we need to filter to less
  // than half the output signal's bw to avoid aliasing, so
  // the half-band here is 0.5*rate.
  double percent = 0.80;

  if (arb_rate <= 1) {
    double halfband = 0.5 * arb_rate;
    double bw       = percent * halfband;
    double tb       = (percent / 2.0) * halfband;

    // BOOST_LOG_TRIVIAL(info) << "Arb Rate: " << arb_rate << " Half band: " << halfband << " bw: " << bw << " tb: " <<
    // tb;

    // As we drop the bw factor, the optfir filter has a harder time converging;
    // using the firdes method here for better results.
    arb_taps = gr::filter::firdes::low_pass_2(arb_size, arb_size, bw, tb, arb_atten,
                                              gr::filter::firdes::WIN_BLACKMAN_HARRIS);
    double tap_total = inital_lpf_taps.size() + channel_lpf_taps.size() + arb_taps.size();
    BOOST_LOG_TRIVIAL(info) << "P25 Recorder Taps - initial: " << inital_lpf_taps.size() << " channel: " << channel_lpf_taps.size() << " ARB: " << arb_taps.size() << " Total: " << tap_total;
  } else {
    BOOST_LOG_TRIVIAL(error) << "Something is probably wrong! Resampling rate too low";
    exit(0);
  }


  arb_resampler = gr::filter::pfb_arb_resampler_ccf::make(arb_rate, arb_taps);


  gr::digital::fll_band_edge_cc::sptr carriertrack = gr::digital::fll_band_edge_cc::make(system_channel_rate, 0.6, 64, 0.35);

  gr::analog::pll_freqdet_cf::sptr pll_demod = gr::analog::pll_freqdet_cf::make(
    2.0 / samples_per_symbol,
    1 * pi / samples_per_symbol,
    -1 * pi / samples_per_symbol);


  gr::digital::clock_recovery_mm_ff::sptr softbits =
    gr::digital::clock_recovery_mm_ff::make(samples_per_symbol,
                                            0.25 * gain_mu * gain_mu,
                                            mu,
                                            gain_mu,
                                            omega_relative_limit);

  gr::digital::binary_slicer_fb::sptr slicer =  gr::digital::binary_slicer_fb::make();

  gr::digital::correlate_access_code_tag_bb::sptr start_correlator =
    gr::digital::correlate_access_code_tag_bb::make("10101100", 0, "smartnet_preamble");


  smartnet_decode_sptr decode = smartnet_make_decode(queue, sys_id);

  connect(self(),           0, prefilter,        0);
  connect(prefilter,        0, channel_lpf,      0);
  connect(channel_lpf,      0, arb_resampler,    0);
  connect(arb_resampler,    0, carriertrack,     0);
  connect(carriertrack,     0, pll_demod,        0);
  connect(pll_demod,        0, softbits,         0);
  connect(softbits,         0, slicer,           0);
  connect(slicer,           0, start_correlator, 0);
  connect(start_correlator, 0, decode,           0);
}


void smartnet_trunking::tune_offset(double f) {
  chan_freq = f;
  int offset_amount = (f - center_freq);
  prefilter->set_center_freq(offset_amount);
  cout << "Offset set to: " << offset_amount << " Freq: " << chan_freq << endl;
}
