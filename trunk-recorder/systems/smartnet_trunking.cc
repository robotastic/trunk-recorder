#include "smartnet_trunking.h"
#include "../formatter.h"

using namespace std;

smartnet_trunking_sptr make_smartnet_trunking(float               freq,
                                              float               center,
                                              long                samp,
                                              gr::msg_queue::sptr queue,
                                              int                 sys_num)
{
  return gnuradio::get_initial_sptr(new smartnet_trunking(freq, center, samp,
                                                          queue, sys_num));
}


void smartnet_trunking::generate_arb_taps() {

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
  arb_taps = gr::filter::firdes::low_pass_2(arb_size, arb_size, bw, tb, arb_atten, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
} else {
  BOOST_LOG_TRIVIAL(error) << "Something is probably wrong! Resampling rate too low";
  exit(1);
}
}

smartnet_trunking::DecimSettings smartnet_trunking::get_decim(long speed) {
    long s = speed;
    long if_freqs[] = {18000, 24000, 25000, 32000};
    DecimSettings decim_settings={-1,-1};
    for (int i = 0; i<3; i++) {
        long if_freq = if_freqs[i];
        if (s % if_freq != 0) {
            continue;
        }
        long q = s / if_freq;
        if (q & 1) {
            continue;
        }

        if ((q >= 40) && ((q & 3) ==0)) {
            decim_settings.decim = q/4;
            decim_settings.decim2 = 4;
        } else {
            decim_settings.decim = q/2;
            decim_settings.decim2 = 2;
        }
        std::cout << "Decim: " << decim_settings.decim << " Decim2:  " << decim_settings.decim2 << std::endl;
        return decim_settings;
    }
    std::cout << "Nothing found" << std::endl;
    return decim_settings;
}
void smartnet_trunking::initialize_prefilter() {
  symbol_rate         = 3600;
  samples_per_symbol  = 5; // was 10
  system_channel_rate = symbol_rate * samples_per_symbol;
  long if_rate = system_channel_rate;
  long fa = 0;
  long fb = 0;
  if1 = 0;
  if2 = 0;




  lo = gr::analog::sig_source_c::make(input_rate, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
  mixer = gr::blocks::multiply_cc::make();

  DecimSettings decim_settings = get_decim(input_rate);
  if (decim_settings.decim != -1) {
    double_decim = true;
    decim = decim_settings.decim;
    if1 = input_rate / decim_settings.decim;
    if2 = if1 / decim_settings.decim2;
    fa = 6250;
    fb = if2 / 2;
    BOOST_LOG_TRIVIAL(info) << "\t smartnet Trunking two-stage decimator - Initial decimated rate: "<< if1 << " Second decimated rate: " << if2  << " FA: " << fa << " FB: " << fb << " System Rate: " << input_rate;
    bandpass_filter_coeffs = gr::filter::firdes::complex_band_pass(1.0, input_rate, -if1/2, if1/2, if1/2);
    lowpass_filter_coeffs = gr::filter::firdes::low_pass(1.0, if1, (fb+fa)/2, fb-fa);
    bandpass_filter = gr::filter::fft_filter_ccc::make(decim_settings.decim, bandpass_filter_coeffs);
    lowpass_filter = gr::filter::fft_filter_ccf::make(decim_settings.decim2, lowpass_filter_coeffs);
    resampled_rate = if2;
    bfo = gr::analog::sig_source_c::make(if1, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
    bandpass_filter->set_max_output_buffer(4096);
    bfo->set_max_output_buffer(4096);
  } else {
    double_decim = false;
    BOOST_LOG_TRIVIAL(info) << "\t smartnet Trunking single-stage decimator - Initial decimated rate: "<< if1 << " Second decimated rate: " << if2  << " Initial Decimation: " << decim << " System Rate: " << input_rate;
    lo = gr::analog::sig_source_c::make(input_rate, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
    lowpass_filter_coeffs = gr::filter::firdes::low_pass(1.0, input_rate, 7250, 1450);
    decim = floor(input_rate / if_rate);
    resampled_rate = input_rate / decim;

    lowpass_filter = gr::filter::fft_filter_ccf::make(decim, lowpass_filter_coeffs);
    resampled_rate = input_rate / decim;
    lo->set_max_output_buffer(4096);
  }

  // Cut-Off Filter
  fa = 6250;
  fb = fa + 625;
  cutoff_filter_coeffs = gr::filter::firdes::low_pass(1.0, if_rate, (fb+fa)/2, fb-fa);
  cutoff_filter = gr::filter::fft_filter_ccf::make(1.0, cutoff_filter_coeffs);

  // ARB Resampler
  arb_rate = if_rate / resampled_rate;
  generate_arb_taps();
  arb_resampler = gr::filter::pfb_arb_resampler_ccf::make(arb_rate, arb_taps);
  BOOST_LOG_TRIVIAL(info) << "\t smartnet Trunking ARB - Initial Rate: "<< input_rate << " Resampled Rate: " << resampled_rate  << " Initial Decimation: " << decim << " System Rate: " << system_channel_rate << " ARB Rate: " << arb_rate;

  mixer->set_max_output_buffer(4096);
  lowpass_filter->set_max_output_buffer(4096);
  arb_resampler->set_max_output_buffer(4096);
  cutoff_filter->set_max_output_buffer(4096);

  if (double_decim) {
    connect(self(), 0, bandpass_filter, 0);
    connect(bandpass_filter, 0, mixer,   0);
    connect(bfo,    0,  mixer,           1);
  } else {
    connect(self(),  0,  mixer,           0);
    connect(lo,     0,  mixer,           1);
  }
  connect(mixer,    0,  lowpass_filter,   0);
  connect(lowpass_filter, 0,  arb_resampler, 0);
  connect(arb_resampler,  0,  cutoff_filter,  0);
}



smartnet_trunking::smartnet_trunking(float               f,
                                     float               c,
                                     long                s,
                                     gr::msg_queue::sptr queue,
                                     int                 sys_num)
  : gr::hier_block2("smartnet_trunking",
                    gr::io_signature::make(1, 1, sizeof(gr_complex)),
                    gr::io_signature::make(0, 0, sizeof(float)))
{
  center_freq  = c;
  chan_freq    = f;
  input_rate    = s;

  this->sys_num = sys_num;



  float gain_mu              = 0.01;
  float mu                   = 0.5;
  float omega_relative_limit = 0.3;

  const double pi = boost::math::constants::pi<double>();
  BOOST_LOG_TRIVIAL(info) <<  "SmartNet Trunking - SysNum: " << sys_num;

  BOOST_LOG_TRIVIAL(info) <<  "Control channel: " << FormatFreq(chan_freq);

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

  slicer =  gr::digital::binary_slicer_fb::make();

  start_correlator =  gr::digital::correlate_access_code_tag_bb::make("10101100", 0, "smartnet_preamble");



  smartnet_decode_sptr decode = smartnet_make_decode(queue, sys_num);

  carriertrack->set_max_output_buffer(4096);
  pll_demod->set_max_output_buffer(4096);
  softbits->set_max_output_buffer(4096);
  slicer->set_max_output_buffer(4096);
  start_correlator->set_max_output_buffer(4096);

  connect(cutoff_filter,    0, carriertrack,     0);
  connect(carriertrack,     0, pll_demod,        0);
  connect(pll_demod,        0, softbits,         0);
  connect(softbits,         0, slicer,           0);
  connect(slicer,           0, start_correlator, 0);
  connect(start_correlator, 0, decode,           0);
  tune_freq(chan_freq);
}

void smartnet_trunking::reset() {
  BOOST_LOG_TRIVIAL(info) << "Pll Phase: " << pll_demod->get_phase() << " min Freq: " << FormatFreq(pll_demod->get_min_freq()) << " Max Freq: " << FormatFreq(pll_demod->get_max_freq());
  carriertrack->set_rolloff(0.6);
  pll_demod->update_gains();
  //pll_demod->frequency_limit();
  pll_demod->phase_wrap();
  softbits->set_verbose(true);
  //pll_demod->set_phase(0);

}

void smartnet_trunking::set_center(double c) {
  center_freq = c;
}

void smartnet_trunking::set_rate(long s) {
  input_rate = s;
  // TODO: Update/remake blocks that depend on input_rate
}

void smartnet_trunking::tune_freq(double f) {
  chan_freq = f;
  int offset_amount = (center_freq - f);
  tune_offset(offset_amount);
}

void smartnet_trunking::tune_offset(double f) {

        float freq = static_cast<float> (f);

        if (abs(freq) > ((input_rate/2) - (if1/2)))
        {
          BOOST_LOG_TRIVIAL(info) << "Tune Offset: Freq exceeds limit: " << abs(freq) << " compared to: " << ((input_rate/2) - (if1/2));
        }
        if (double_decim) {
          bandpass_filter_coeffs = gr::filter::firdes::complex_band_pass(1.0, input_rate, -freq - if1/2, -freq + if1/2, if1/2);
          bandpass_filter->set_taps(bandpass_filter_coeffs);
          float bfz = (static_cast<float> (decim) * -freq) / (float) input_rate;
          bfz = bfz - static_cast<int>(bfz);
          if (bfz < -0.5) {
            bfz = bfz + 1.0;
          }
          if (bfz > 0.5) {
            bfz = bfz - 1.0;
          }
          bfo->set_frequency(-bfz * if1);
        } else {
          lo->set_frequency(freq);
        }
}
