#include "smartnet_trunking.h"
#include "../formatter.h"

using namespace std;

smartnet_trunking_sptr make_smartnet_trunking(double freq,
                                              gr::msg_queue::sptr queue,
                                              int sys_num) {
  return gnuradio::get_initial_sptr(new smartnet_trunking(freq,
                                                          queue, sys_num));
}

void smartnet_trunking::generate_arb_taps() {

  double arb_size = 32;
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
    double bw = percent * halfband;
    double tb = (percent / 2.0) * halfband;

// BOOST_LOG_TRIVIAL(info) << "Arb Rate: " << arb_rate << " Half band: " << halfband << " bw: " << bw << " tb: " <<
// tb;

// As we drop the bw factor, the optfir filter has a harder time converging;
// using the firdes method here for better results.
#if GNURADIO_VERSION < 0x030900
    arb_taps = gr::filter::firdes::low_pass_2(arb_size, arb_size, bw, tb, arb_atten, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
#else
    arb_taps = gr::filter::firdes::low_pass_2(arb_size, arb_size, bw, tb, arb_atten, gr::fft::window::WIN_BLACKMAN_HARRIS);
#endif
  } else {
    BOOST_LOG_TRIVIAL(error) << "Something is probably wrong! Resampling rate too low";
    exit(1);
  }
}

void smartnet_trunking::initialize_prefilter() {
  symbol_rate = 3600;
  samples_per_symbol = 5; // was 10
  system_channel_rate = symbol_rate * samples_per_symbol;
  long if_rate = system_channel_rate;
  long fa = 0;
  long fb = 0;

  valve = gr::blocks::copy::make(sizeof(gr_complex));
  valve->set_enabled(false);

  // Cut-Off Filter
  fa = 6250;
  fb = fa + 625;
  cutoff_filter_coeffs = gr::filter::firdes::low_pass(1.0, if_rate, (fb + fa) / 2, fb - fa);
  cutoff_filter = gr::filter::fft_filter_ccf::make(1.0, cutoff_filter_coeffs);

  // ARB Resampler
  arb_rate = if_rate / 25000.0;
  generate_arb_taps();
  arb_resampler = gr::filter::pfb_arb_resampler_ccf::make(arb_rate, arb_taps);
  BOOST_LOG_TRIVIAL(info) << "\t smartnet Trunking ARB - Initial Rate: " << input_rate << " Resampled Rate: " << resampled_rate << " Initial Decimation: " << decim << " System Rate: " << system_channel_rate << " ARB Rate: " << arb_rate;

  connect(self(), 0, valve, 0);
  connect(valve, 0, arb_resampler, 0);
  connect(arb_resampler, 0, cutoff_filter, 0);
}

smartnet_trunking::smartnet_trunking(double f,
                                     gr::msg_queue::sptr queue,
                                     int sys_num)
    : gr::hier_block2("smartnet_trunking",
                      gr::io_signature::make(1, 1, sizeof(gr_complex)),
                      gr::io_signature::make(0, 0, sizeof(float))) {

  chan_freq = f;

  this->sys_num = sys_num;

  float gain_mu = 0.01;
  float mu = 0.5;
  float omega_relative_limit = 0.3;

  const double pi = boost::math::constants::pi<double>();
  BOOST_LOG_TRIVIAL(info) << "SmartNet Trunking - SysNum: " << sys_num;

  BOOST_LOG_TRIVIAL(info) << "Control channel: " << format_freq(chan_freq);

  initialize_prefilter();

  carriertrack = gr::digital::fll_band_edge_cc::make(system_channel_rate, 0.6, 64, 0.35);

  pll_demod = gr::analog::pll_freqdet_cf::make(
      2.0 / samples_per_symbol,
      1 * pi / samples_per_symbol,
      -1 * pi / samples_per_symbol);

  softbits = gr::digital::clock_recovery_mm_ff::make(samples_per_symbol,
                                                     0.25 * gain_mu * gain_mu,
                                                     mu,
                                                     gain_mu,
                                                     omega_relative_limit);

  slicer = gr::digital::binary_slicer_fb::make();

  start_correlator = gr::digital::correlate_access_code_tag_bb::make("10101100", 0, "smartnet_preamble");

  smartnet_decode_sptr decode = smartnet_make_decode(queue, sys_num);

  connect(cutoff_filter, 0, carriertrack, 0);
  connect(carriertrack, 0, pll_demod, 0);
  connect(pll_demod, 0, softbits, 0);
  connect(softbits, 0, slicer, 0);
  connect(slicer, 0, start_correlator, 0);
  connect(start_correlator, 0, decode, 0);
}

void smartnet_trunking::reset() {
  BOOST_LOG_TRIVIAL(info) << "Pll Phase: " << pll_demod->get_phase() << " min Freq: " << format_freq(pll_demod->get_min_freq()) << " Max Freq: " << format_freq(pll_demod->get_max_freq());
  carriertrack->set_rolloff(0.6);
  pll_demod->update_gains();
  // pll_demod->frequency_limit();
  pll_demod->phase_wrap();
  // softbits->set_verbose(true);
  // pll_demod->set_phase(0);
}


double smartnet_trunking::get_freq() {
  return chan_freq;
}

void smartnet_trunking::start() {
  valve->set_enabled(true);
}

void smartnet_trunking::stop() {
  valve->set_enabled(false);
} 
