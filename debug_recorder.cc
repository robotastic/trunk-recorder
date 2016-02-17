
#include "debug_recorder.h"
using namespace std;

bool debug_recorder::logging = false;

debug_recorder_sptr make_debug_recorder(Source *src, long t, int n)
{
	return gnuradio::get_initial_sptr(new debug_recorder(src, t, n));
}

debug_recorder::debug_recorder(Source *src, long t, int n)
	: gr::hier_block2 ("debug_recorder",
	                   gr::io_signature::make  (1, 1, sizeof(gr_complex)),
	                   gr::io_signature::make  (0, 0, sizeof(float)))
{
    source = src;
	freq = source->get_center();
	center = source->get_center();
	samp_rate = source->get_rate();
	talkgroup = t;
	num = n;
	active = false;


	starttime = time(NULL);

	float offset = 0; //have to flip for 3.7

	int samp_per_sym = 10;
    float symbol_rate = 4800;
	double decim = floor(samp_rate / 100000);
	float xlate_bandwidth = 25000; //14000; //24260.0;
	float channel_rate = 4800 * samp_per_sym;
	double pre_channel_rate = samp_rate/decim;
    const double pi = M_PI;

            float if_rate = 48000;
        float gain_mu = 0.025;
        float costas_alpha = 0.04;
        double sps = 0.0;
        float bb_gain = 1.0;

            lpf_coeffs = gr::filter::firdes::low_pass(1.0, samp_rate, xlate_bandwidth/2, 1500, gr::filter::firdes::WIN_HANN);
        int decimation = int(samp_rate / if_rate);

        prefilter = gr::filter::freq_xlating_fir_filter_ccf::make(decimation,
	            lpf_coeffs,
	            offset,
	            samp_rate);
        
        float resampled_rate = float(samp_rate) / float(decimation); // rate at output of self.lpf


        float arb_rate = (float(if_rate) / resampled_rate);
        float arb_size = 32;
        float arb_atten=100;


            // Create a filter that covers the full bandwidth of the output signal

            // If rate >= 1, we need to prevent images in the output,
            // so we have to filter it to less than half the channel
            // width of 0.5.  If rate < 1, we need to filter to less
            // than half the output signal's bw to avoid aliasing, so
            // the half-band here is 0.5*rate.
            float percent = 0.80;
            if(arb_rate < 1) {
                float halfband = 0.5* arb_rate;
                float bw = percent*halfband;
                float tb = (percent/2.0)*halfband;
                float ripple = 0.1;

                // As we drop the bw factor, the optfir filter has a harder time converging;
                // using the firdes method here for better results.
                arb_taps = gr::filter::firdes::low_pass_2(arb_size, arb_size, bw, tb, arb_atten,
                                                      gr::filter::firdes::WIN_BLACKMAN_HARRIS);
            } else {
                BOOST_LOG_TRIVIAL(error) << "CRAP! Computer over!";
            	/*
                float halfband = 0.5;
                float bw = percent*halfband;
                float tb = (percent/2.0)*halfband;
                float ripple = 0.1;

                bool made = False;
                while not made:
                    try:
                        self._taps = optfir.low_pass(self._size, self._size, bw, bw+tb, ripple, atten)
                        made = True
                    except RuntimeError:
                        ripple += 0.01
                        made = False
                        print("Warning: set ripple to %.4f dB. If this is a problem, adjust the attenuation or create your own filter taps." % (ripple))

                        # Build in an exit strategy; if we've come this far, it ain't working.
                        if(ripple >= 1.0):
                            raise RuntimeError("optfir could not generate an appropriate filter.")*/
            }






        arb_resampler = gr::filter::pfb_arb_resampler_ccf::make(arb_rate, arb_taps );


        agc = gr::analog::feedforward_agc_cc::make(16, 1.0);


        float omega = float(if_rate) / float(symbol_rate);
        float gain_omega = 0.1  * gain_mu * gain_mu;

        float alpha = costas_alpha;
        float beta = 0.125 * alpha * alpha;
        float fmax = 2400;	// Hz
        fmax = 2*pi * fmax / float(if_rate);

        costas_clock = gr::op25_repeater::gardner_costas_cc::make(omega, gain_mu, gain_omega, alpha,  beta, fmax, -fmax);
       

        // Perform Differential decoding on the constellation
        diffdec = gr::digital::diff_phasor_cc::make();

        // take angle of the difference (in radians)
        to_float = gr::blocks::complex_to_arg::make();
        
        

        valve = gr::blocks::copy::make(sizeof(gr_complex));
        valve->set_enabled(false);


        tm *ltm = localtime(&starttime);

        std::stringstream path_stream;
        path_stream << boost::filesystem::current_path().string() <<  "/debug";

        boost::filesystem::create_directories(path_stream.str());
        sprintf(filename, "%s/%ld-%ld_%g.raw", path_stream.str().c_str(),talkgroup,starttime,freq);
        raw_sink = gr::blocks::file_sink::make(sizeof(float), filename);


		connect(self(),0, valve,0);
		connect(valve,0, prefilter,0);
		connect(prefilter, 0, arb_resampler, 0);
		connect(arb_resampler,0, agc,0);
        connect(agc, 0, costas_clock, 0);
		connect(costas_clock,0, diffdec, 0);
		connect(diffdec, 0, to_float, 0);
        connect(to_float,0, raw_sink,0);
        
}

debug_recorder::~debug_recorder() {

}


bool debug_recorder::is_active() {
	return active;
}

long debug_recorder::get_talkgroup() {
	return talkgroup;
}

double debug_recorder::get_freq() {
	return freq;
}

Source *debug_recorder::get_source() {
    return source;
}

char *debug_recorder::get_filename() {
	return filename;
}

void debug_recorder::tune_offset(double f) {
	freq = f;
	long offset_amount = (f - center);
	prefilter->set_center_freq(offset_amount); // have to flip this for 3.7
}
void debug_recorder::deactivate() {
	BOOST_LOG_TRIVIAL(info) << "debug_recorder.cc: Deactivating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]";


	raw_sink->close();



	active = false;
	valve->set_enabled(false);




}

void debug_recorder::activate( long t, double f, int n, char *existing_filename) {

	starttime = time(NULL);

	talkgroup = t;
	freq = f;

	tm *ltm = localtime(&starttime);
	BOOST_LOG_TRIVIAL(info) << "debug_recorder.cc: Activating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]";


	prefilter->set_center_freq(f - center); // have to flip for 3.7
	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/debug";

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.raw", path_stream.str().c_str(),talkgroup,starttime,freq);


	raw_sink->open(filename);


	active = true;
	valve->set_enabled(true);
}
