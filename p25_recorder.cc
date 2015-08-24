
#include "p25_recorder.h"
#include <boost/log/trivial.hpp>


p25_recorder_sptr make_p25_recorder(double freq, double center, long s, long t, int n)
{
	return gnuradio::get_initial_sptr(new p25_recorder(freq, center, s, t, n));
}


unsigned p25_recorder::GCD(unsigned u, unsigned v) {
	while ( v != 0) {
		unsigned r = u % v;
		u = v;
		v = r;
	}
	return u;
}

std::vector<float> p25_recorder::design_filter(double interpolation, double deci) {
	float beta = 5.0;
	float trans_width = 0.5 - 0.4;
	float mid_transition_band = 0.5 - trans_width/2;

	std::vector<float> result = gr::filter::firdes::low_pass(
	                                interpolation,
	                                1,
	                                mid_transition_band/interpolation,
	                                trans_width/interpolation,
	                                gr::filter::firdes::WIN_KAISER,
	                                beta
	                            );

	return result;
}



p25_recorder::p25_recorder(double f, double c, long s, long t, int n)
	: gr::hier_block2 ("p25_recorder",
	                   gr::io_signature::make  (1, 1, sizeof(gr_complex)),
	                   gr::io_signature::make  (0, 0, sizeof(float)))
{
	freq = f;
	center = c;
	talkgroup = t;
	long capture_rate = s;
	num = n;
	active = false;

	float offset = f - center;



	float symbol_rate = 4800;
	double samples_per_symbol = 10;
	double system_channel_rate = symbol_rate * samples_per_symbol;
	double prechannel_decim = floor(capture_rate / system_channel_rate);
	double prechannel_rate = capture_rate / prechannel_decim;
	double trans_width = 12500 / 2;
	double trans_centre = trans_width + (trans_width / 2);
	float symbol_deviation = 600.0;
	bool fsk4 = true;

	std::vector<float> sym_taps;
	const double pi = M_PI; //boost::math::constants::pi<double>();

	timestamp = time(NULL);
	starttime = time(NULL);

        double input_rate = capture_rate;
        float if_rate = 48000;
        float gain_mu = 0.025;
        float costas_alpha = 0.04;
        double sps = 0.0;
        float bb_gain = 1.0;

       	baseband_amp = gr::blocks::multiply_const_ff::make(bb_gain);

        // local osc
        lo = gr::analog::sig_source_c::make(input_rate, gr::analog::GR_SIN_WAVE, 0, 1.0, 0);
        mixer = gr::blocks::multiply_cc::make();
        lpf_coeffs = gr::filter::firdes::low_pass(1.0, input_rate, 15000, 1500, gr::filter::firdes::WIN_HANN);
        int decimation = int(input_rate / if_rate);
        lpf = gr::filter::fir_filter_ccf::make(decimation, lpf_coeffs);

        float resampled_rate = float(input_rate) / float(decimation); // rate at output of self.lpf


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





        float omega = float(if_rate) / float(symbol_rate);
        float gain_omega = 0.1  * gain_mu * gain_mu;

        float alpha = costas_alpha;
        float beta = 0.125 * alpha * alpha;
        float fmax = 2400;	// Hz
        fmax = 2*pi * fmax / float(if_rate);

        costas_clock = gr::op25_repeater::gardner_costas_cc::make(omega, gain_mu, gain_omega, alpha,  beta, fmax, -fmax);

        agc = gr::analog::feedforward_agc_cc::make(16, 1.0);

        // Perform Differential decoding on the constellation
        diffdec = gr::digital::diff_phasor_cc::make();

        // take angle of the difference (in radians)
        to_float = gr::blocks::complex_to_arg::make();

        // convert from radians such that signal is in -3/-1/+1/+3
        rescale = gr::blocks::multiply_const_ff::make( (1 / (pi / 4)) );

        // fm demodulator (needed in fsk4 case)
        float fm_demod_gain = if_rate / (2.0 * pi * symbol_deviation);
        fm_demod = gr::analog::quadrature_demod_cf::make(fm_demod_gain);



	double symbol_decim = 1;

	valve = gr::blocks::copy::make(sizeof(gr_complex));
	valve->set_enabled(false);

	BOOST_LOG_TRIVIAL(info) << " FM Gain: " << fm_demod_gain << " PI: " << pi << " Samples per sym: " << samples_per_symbol;

	for (int i=0; i < samples_per_symbol; i++) {
		sym_taps.push_back(1.0 / samples_per_symbol);
	}
	//symbol_coeffs = (1.0/samples_per_symbol,)*samples_per_symbol
	sym_filter =  gr::filter::fir_filter_fff::make(symbol_decim, sym_taps);
	tune_queue = gr::msg_queue::make(2);
	traffic_queue = gr::msg_queue::make(2);
	rx_queue = gr::msg_queue::make(100);
	const float l[] = { -2.0, 0.0, 2.0, 4.0 };
	std::vector<float> levels( l,l + sizeof( l ) / sizeof( l[0] ) );
	fsk4_demod = gr::op25::fsk4_demod_ff::make(tune_queue, system_channel_rate, symbol_rate);
	slicer = gr::op25_repeater::fsk4_slicer_fb::make(levels);

	int udp_port = 0;
	int verbosity = 10;
	const char * wireshark_host="127.0.0.1";
	bool do_imbe = 0;
	bool do_output = 1;
	bool do_msgq = 0;
	bool do_audio_output = 1;
	bool do_tdma = 0;
	op25_frame_assembler = gr::op25_repeater::p25_frame_assembler::make(wireshark_host,udp_port,verbosity,do_imbe, do_output, do_msgq, rx_queue, do_audio_output, do_tdma);
	//op25_vocoder = gr::op25_repeater::vocoder::make(0, 0, 0, "", 0, 0);

	converter = gr::blocks::short_to_float::make();
	//converter = gr::blocks::char_to_float::make();
	float convert_num = float(1.0)/float(32768.0);
	multiplier = gr::blocks::multiply_const_ff::make(convert_num);
	tm *ltm = localtime(&starttime);

	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.wav", path_stream.str().c_str(),talkgroup,timestamp,freq);
	wav_sink = gr::blocks::nonstop_wavfile_sink::make(filename,1,8000,16);





	if (fsk4) {
		connect(self(),0, mixer, 0);
		connect(lo,0, mixer, 1);
		connect(mixer,0, valve,0);
		connect(valve, 0, lpf, 0);
		connect(lpf, 0, arb_resampler, 0);
		connect(arb_resampler,0, fm_demod,0);
		connect(fm_demod, 0, baseband_amp, 0);
		connect(baseband_amp,0, sym_filter, 0);
		connect(sym_filter, 0, fsk4_demod, 0);
		connect(fsk4_demod, 0, slicer, 0);
		connect(slicer,0, op25_frame_assembler,0);
		connect(op25_frame_assembler, 0,  converter,0);
		connect(converter, 0, multiplier,0);
		connect(multiplier, 0, wav_sink,0);
	} else {
		connect(self(),0, mixer, 0);
		connect(lo,0, mixer, 1);
		connect(mixer,0, valve,0);
		connect(valve, 0, lpf, 0);
		connect(lpf, 0, arb_resampler, 0);
		connect(arb_resampler,0, agc,0);
		connect(agc, 0, costas_clock, 0);
		connect(costas_clock,0, diffdec, 0);
		connect(diffdec, 0, to_float, 0);
		connect(to_float,0, rescale, 0);
		connect(rescale, 0, slicer, 0);
		connect(slicer,0, op25_frame_assembler,0);
		connect(op25_frame_assembler, 0,  converter,0);
		connect(converter, 0, multiplier,0);
		connect(multiplier, 0, wav_sink,0);
	}
}


p25_recorder::~p25_recorder() {

}


bool p25_recorder::is_active() {
	return active;
}


double p25_recorder::get_freq() {
	return freq;
}

char *p25_recorder::get_filename() {
	return filename;
}

int p25_recorder::lastupdate() {
	return time(NULL) - timestamp;
}

long p25_recorder::elapsed() {
	return time(NULL) - starttime;
}


void p25_recorder::tune_offset(double f) {
	freq = f;
	int offset_amount = (f - center);
	lo->set_frequency(-offset_amount);
	//prefilter->set_center_freq(offset_amount); // have to flip this for 3.7
	//BOOST_LOG_TRIVIAL(info) << "Offset set to: " << offset_amount << " Freq: "  << freq;
}

void p25_recorder::deactivate() {
	BOOST_LOG_TRIVIAL(info) << "p25_recorder.cc: Deactivating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]";

	active = false;
	valve->set_enabled(false);
	wav_sink->close();
}

void p25_recorder::activate(long t, double f, int n) {

	timestamp = time(NULL);
	starttime = time(NULL);

	talkgroup = t;
	freq = f;

	tm *ltm = localtime(&starttime);
	BOOST_LOG_TRIVIAL(info) << "p25_recorder.cc: Activating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]";

	int offset_amount = (f - center);
	lo->set_frequency(-offset_amount);


	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.wav", path_stream.str().c_str(),talkgroup,starttime,f);

	wav_sink->open(filename);
	active = true;
	valve->set_enabled(true);
}


