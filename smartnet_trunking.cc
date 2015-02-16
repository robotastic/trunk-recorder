#include "smartnet_trunking.h"

using namespace std;

smartnet_trunking_sptr make_smartnet_trunking(float freq, float center, long samp, gr::msg_queue::sptr queue)
{
	return gnuradio::get_initial_sptr(new smartnet_trunking(freq, center, samp, queue));
}

smartnet_trunking::smartnet_trunking(float f, float c, long s, gr::msg_queue::sptr queue)
	: gr::hier_block2 ("smartnet_trunking",
	                   gr::io_signature::make  (1, 1, sizeof(gr_complex)),
	                   gr::io_signature::make  (0, 0, sizeof(float)))
{
	center_freq = c;
	chan_freq = f;
	samp_rate = s;
	float samples_per_second = samp_rate;
	float syms_per_sec = 3600;
	float gain_mu = 0.01;
	float mu=0.5;
	float omega_relative_limit = 0.3;
	float offset = chan_freq - center_freq;  // have to reverse it for 3.7 because it swapped in the switch.
	float clockrec_oversample = 3;
	int decim = int(samples_per_second / (syms_per_sec * clockrec_oversample));
	float sps = samples_per_second/decim/syms_per_sec;
	const double pi = boost::math::constants::pi<double>();

	cout << "Control channel offset: " << offset << endl;
	cout << "Control channel: " << chan_freq << endl;
	cout << "Decim: " << decim << endl;
	cout << "Samples per symbol: " << sps << endl;

	std::vector<float> lpf_taps;


	lpf_taps =  gr::filter::firdes::low_pass(1, samp_rate, 4500, 2000);

	gr::filter::freq_xlating_fir_filter_ccf::sptr prefilter = gr::filter::freq_xlating_fir_filter_ccf::make(decim,
	        lpf_taps,
	        offset,
	        samp_rate);

	gr::digital::fll_band_edge_cc::sptr carriertrack = gr::digital::fll_band_edge_cc::make(sps, 0.6, 64, 0.35);

	gr::analog::pll_freqdet_cf::sptr pll_demod = gr::analog::pll_freqdet_cf::make(2.0 / clockrec_oversample, 1*pi/clockrec_oversample, -1*pi/clockrec_oversample);

	gr::digital::clock_recovery_mm_ff::sptr softbits = gr::digital::clock_recovery_mm_ff::make(sps, 0.25 * gain_mu * gain_mu, mu, gain_mu, omega_relative_limit);

	gr::digital::binary_slicer_fb::sptr slicer =  gr::digital::binary_slicer_fb::make();

	gr::digital::correlate_access_code_tag_bb::sptr start_correlator = gr::digital::correlate_access_code_tag_bb::make("10101100",0,"smartnet_preamble");

	smartnet_deinterleave_sptr deinterleave = smartnet_make_deinterleave();

	smartnet_crc_sptr crc = smartnet_make_crc(queue);

	connect(self(),0,prefilter,0);
	connect(prefilter,0,carriertrack,0);
	connect(carriertrack, 0, pll_demod, 0);
	connect(pll_demod, 0, softbits, 0);
	connect(softbits, 0, slicer, 0);
	connect(slicer, 0, start_correlator, 0);
	connect(start_correlator, 0, deinterleave, 0);
	connect(deinterleave, 0, crc, 0);
}