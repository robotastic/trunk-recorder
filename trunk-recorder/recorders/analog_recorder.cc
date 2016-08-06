
#include "analog_recorder.h"
using namespace std;

bool analog_recorder::logging = false;

analog_recorder_sptr make_analog_recorder(Source *src)
{
	return gnuradio::get_initial_sptr(new analog_recorder(src));
}


analog_recorder::analog_recorder(Source *src)
	: gr::hier_block2 ("analog_recorder",
	                   gr::io_signature::make  (1, 1, sizeof(gr_complex)),
	                   gr::io_signature::make  (0, 0, sizeof(float)))
{
    source = src;
	freq = source->get_center();
	center = source->get_center();
	samp_rate = source->get_rate();
	talkgroup = 0;
	num = 0;
	active = false;

	timestamp = time(NULL);
	starttime = time(NULL);

	float offset = 0; //have to flip for 3.7

	int samp_per_sym = 10;
	double decim = 80;
	float xlate_bandwidth = 14000; //24260.0;
	float channel_rate = 4800 * samp_per_sym;
	double pre_channel_rate = samp_rate/decim;

	lpf_taps =  gr::filter::firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 6000);
	//lpf_taps =  gr::filter::firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 3000);

	std::vector<gr_complex> dest(lpf_taps.begin(), lpf_taps.end());

					prefilter = make_freq_xlating_fft_filter(decim,
		            dest,
		            offset,
		            samp_rate);
/*
	prefilter = gr::filter::freq_xlating_fir_filter_ccf::make(decim,
	            lpf_taps,
	            offset,
	            samp_rate);*/
	unsigned int d = GCD(channel_rate, pre_channel_rate); //4000 GCD(48000, 100000)
	channel_rate = floor(channel_rate  / d);  // 12
	pre_channel_rate = floor(pre_channel_rate / d);  // 25
	resampler_taps = design_filter(channel_rate, pre_channel_rate);

	downsample_sig = gr::filter::rational_resampler_base_ccf::make(channel_rate, pre_channel_rate, resampler_taps); //downsample from 100k to 48k

	//on a trunked network where you know you will have good signal, a carrier power squelch works well. real FM receviers use a noise squelch, where
	//the received audio is high-passed above the cutoff and then fed to a reverse squelch. If the power is then BELOW a threshold, open the squelch.

 	squelch_db = source->get_squelch_db();
         if (squelch_db!=0) {
 		squelch = gr::analog::pwr_squelch_cc::make(squelch_db, 		//squelch point
                                                            0.01,	 	//alpha
 							   10, 		//ramp
 							   false); 	// Non-blocking as we are using squelch_two as a gate.

 		 //  based on squelch code form ham2mon
 		squelch_two = gr::analog::pwr_squelch_ff::make(-200, 	// set low -200 since its after demod and its just gate for previous squelch
 							        0.01, 	//alpha
 					  		        0, 	//ramp
 					   		        true); 	//gated so that the audio recording doesn't contain blank spaces between transmissions
         }


	//k = quad_rate/(2*math.pi*max_dev) = 48k / (6.283185*5000) = 1.527

	demod = gr::analog::quadrature_demod_cf::make(1.527); //1.6 //1.4);
	levels = gr::blocks::multiply_const_ff::make(1); //33);
	valve = gr::blocks::copy::make(sizeof(gr_complex));
	valve->set_enabled(false);

	float tau = 0.000075; //75us
	float w_p = 1/tau;
	float w_pp = tan(w_p / (48000.0*2));

	float a1 = (w_pp - 1)/(w_pp + 1);
	float b0 = w_pp/(1 + w_pp);
	float b1 = b0;

	std::vector<double> btaps(2);// = {b0, b1};
	std::vector<double> ataps(2);// = {1, a1};

	btaps[0] = b0;
	btaps[1] = b1;
	ataps[0] = 1;
	ataps[1] = a1;

	deemph = gr::filter::iir_filter_ffd::make(btaps,ataps);

	audio_resampler_taps = design_filter(1, 6);
	decim_audio = gr::filter::fir_filter_fff::make(6, audio_resampler_taps); //downsample from 48k to 8k


	iam_logging = false;

	tm *ltm = localtime(&starttime);

	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.wav", path_stream.str().c_str(),talkgroup,timestamp,freq);
	sprintf(status_filename, "%s/%ld-%ld_%g.json", path_stream.str().c_str(),talkgroup,timestamp,freq);

	wav_sink = gr::blocks::nonstop_wavfile_sink::make(filename,1,8000,16);



    if (squelch_db!=0) {
 		// using squelch
 		connect(self(),0, valve,0);
 		connect(valve,0, prefilter,0);
 		connect(prefilter, 0, downsample_sig, 0);
 		connect(downsample_sig, 0, squelch, 0);
 		connect(squelch, 0,	demod, 0);
 		connect(demod, 0, deemph, 0);
 		connect(deemph, 0, decim_audio, 0);
 		connect(decim_audio, 0, squelch_two, 0);
 		connect(squelch_two, 0, wav_sink, 0);
 	   } else {
 		// No squelch used
 		connect(self(),0, valve,0);
 		connect(valve,0, prefilter,0);
 		connect(prefilter, 0, downsample_sig, 0);
 		connect(downsample_sig, 0, demod, 0);
 		connect(demod, 0, deemph, 0);
 		connect(deemph, 0, decim_audio, 0);
 		connect(decim_audio, 0, wav_sink, 0);
 	}


}

analog_recorder::~analog_recorder() {

}

Recorder::State analog_recorder::get_state() {
	return state;
}

void analog_recorder::close() {
	if (state == closing) {
		state = inactive;
		valve->set_enabled(false);
		wav_sink->close();
	} else {
		BOOST_LOG_TRIVIAL(info) << "p25_recorder.cc: Closing a non-closing Logger \t[ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]";

	}
}


bool analog_recorder::is_active() {
	return active;
}

long analog_recorder::get_talkgroup() {
	return talkgroup;
}

double analog_recorder::get_freq() {
	return freq;
}

Source *analog_recorder::get_source() {
    return source;
}

double analog_recorder::get_current_length() {
	return wav_sink->length_in_seconds();
}

void analog_recorder::tune_offset(double f) {
	freq = f;
	int offset_amount = (f- center);
	prefilter->set_center_freq(offset_amount); // have to flip this for 3.7
}

void analog_recorder::deactivate() {

	active = false;

	valve->set_enabled(false);

	wav_sink->close();

	ofstream myfile (status_filename);
	if (myfile.is_open())
	{
		myfile << "{\n";
		myfile << "\"freq\": " << freq << ",\n";
		myfile << "\"num\": " << num << ",\n";
		myfile << "\"talkgroup\": " << talkgroup << ",\n";
		myfile << "\"mode\": \"analog\" \n";
		myfile << "}\n";
		myfile.close();
	}
	else cout << "Unable to open file";
}

void analog_recorder::activate(Call *call, int n) {

	starttime = time(NULL);

	talkgroup = call->get_talkgroup();
	freq = call->get_freq();
    num = n;

	prefilter->set_center_freq( freq - center); // have to flip for 3.7

	wav_sink->open(call->get_filename());

	active = true;
	valve->set_enabled(true);
}
