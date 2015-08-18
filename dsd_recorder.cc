
#include "dsd_recorder.h"
using namespace std;

bool dsd_recorder::logging = false;

dsd_recorder_sptr make_dsd_recorder(float freq, float center, long s, long t, int n)
{
	return gnuradio::get_initial_sptr(new dsd_recorder(freq, center, s, t, n));
}

dsd_recorder::dsd_recorder(double f, double c, long s, long t, int n)
	: gr::hier_block2 ("dsd_recorder",
	                   gr::io_signature::make  (1, 1, sizeof(gr_complex)),
	                   gr::io_signature::make  (0, 0, sizeof(float)))
{
	freq = f;
	center = c;
	samp_rate = s;
	talkgroup = t;
	num = n;
	active = false;


	starttime = time(NULL);

	float offset = f - center; //have to flip for 3.7

	int samp_per_sym = 10;
	double decim = 80;
	float xlate_bandwidth = 7000; //14000; //24260.0;
	float channel_rate = 4800 * samp_per_sym;
	double pre_channel_rate = samp_rate/decim;



	lpf_taps =  gr::filter::firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 5000, gr::filter::firdes::WIN_HAMMING);

	prefilter = gr::filter::freq_xlating_fir_filter_ccf::make(decim,
	            lpf_taps,
	            offset,
	            samp_rate);
	unsigned int d = GCD(channel_rate, pre_channel_rate);
	channel_rate = floor(channel_rate  / d);
	pre_channel_rate = floor(pre_channel_rate / d);
	resampler_taps = design_filter(channel_rate, pre_channel_rate);

	downsample_sig = gr::filter::rational_resampler_base_ccf::make(channel_rate, pre_channel_rate, resampler_taps);
	demod = gr::analog::quadrature_demod_cf::make(1.2); //1.6); //1.4);
	levels = gr::blocks::multiply_const_ff::make(1.0); //.40); //33);
	valve = gr::blocks::copy::make(sizeof(gr_complex));
	valve->set_enabled(false);

	for (int i=0; i < samp_per_sym; i++) {
		sym_taps.push_back(1.0 / samp_per_sym);
	}
	sym_filter = gr::filter::fir_filter_fff::make(1, sym_taps);
	lpf_second = gr::filter::fir_filter_fff::make(1,gr::filter::firdes::low_pass(1, 48000, 6000, 500));
	iam_logging = false;
	dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1,dsd_MOD_GFSK,4,0,0, false, num);

	tm *ltm = localtime(&starttime);

	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.wav", path_stream.str().c_str(),talkgroup,starttime,freq);
	sprintf(status_filename, "%s/%ld-%ld_%g.json", path_stream.str().c_str(),talkgroup,starttime,freq);
	wav_sink = gr::blocks::nonstop_wavfile_sink::make(filename,1,8000,16);
	null_sink = gr::blocks::null_sink::make(sizeof(gr_complex));


		connect(self(),0, valve,0);
		connect(valve,0, prefilter,0);
		connect(prefilter, 0, downsample_sig, 0);
		connect(downsample_sig, 0, demod, 0);
		connect(demod, 0, sym_filter, 0);
		connect(sym_filter, 0, levels, 0);
		connect(levels, 0, dsd, 0);
		connect(dsd, 0, wav_sink,0);
}

dsd_recorder::~dsd_recorder() {

}


bool dsd_recorder::is_active() {
	return active;
}

long dsd_recorder::get_talkgroup() {
	return talkgroup;
}

double dsd_recorder::get_freq() {
	return freq;
}

char *dsd_recorder::get_filename() {
	return filename;
}

void dsd_recorder::tune_offset(double f) {
	freq = f;
	long offset_amount = (f - center);
	prefilter->set_center_freq(offset_amount); // have to flip this for 3.7
}
void dsd_recorder::deactivate() {
	BOOST_LOG_TRIVIAL(info) << "dsd_recorder.cc: Deactivating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]";

	//lock();

	wav_sink->close();

/*	disconnect(self(), 0, prefilter, 0);
	connect(self(),0, null_sink,0);

	disconnect(prefilter, 0, downsample_sig, 0);
	disconnect(downsample_sig, 0, demod, 0);
	disconnect(demod, 0, sym_filter, 0);
	disconnect(sym_filter, 0, levels, 0);
	disconnect(levels, 0, dsd, 0);
	disconnect(dsd, 0, wav_sink,0);*/

	active = false;
	valve->set_enabled(false);


	//unlock();


	dsd_state *state = dsd->get_state();
	ofstream myfile (status_filename);
	if (myfile.is_open())
	{
		int level = (int) state->max / 164;
		int index=0;
		myfile << "{\n";
		myfile << "\"freq\": " << freq << ",\n";
		myfile << "\"num\": " << num << ",\n";
		myfile << "\"talkgroup\": " << talkgroup << ",\n";
		myfile << "\"center\": " << state->center << ",\n";
		myfile << "\"umid\": " << state->umid << ",\n";
		myfile << "\"lmid\": " << state->lmid << ",\n";
		myfile << "\"max\": " << state->max << ",\n";
		myfile << "\"inlvl\": " << level << ",\n";
		myfile << "\"nac\": " << state->nac << ",\n";
		myfile << "\"src\": " << state->lastsrc << ",\n";
		myfile << "\"dsdtg\": " << state->lasttg << ",\n";
		myfile << "\"headerCriticalErrors\": " << state->debug_header_critical_errors << ",\n";
		myfile << "\"headerErrors\": " << state->debug_header_errors << ",\n";
		myfile << "\"audioErrors\": " << state->debug_audio_errors << ",\n";
		myfile << "\"symbCount\": " << state->symbolcnt << ",\n";
		myfile << "\"mode\": \"digital\",\n";
		myfile << "\"srcList\": [ ";
		while(state->src_list[index]!=0) {
			if (index !=0) {
				myfile << ", " << state->src_list[index];
			} else {
				myfile << state->src_list[index];
			}
			index++;
		}
		myfile << " ]\n";
		myfile << "}\n";
		myfile.close();
	}
	else BOOST_LOG_TRIVIAL(error) << "Unable to open file";
	dsd->reset_state();
}

void dsd_recorder::activate( long t, double f, int n) {

	starttime = time(NULL);

	talkgroup = t;
	freq = f;

	tm *ltm = localtime(&starttime);
	BOOST_LOG_TRIVIAL(info) << "dsd_recorder.cc: Activating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]";


	prefilter->set_center_freq(f - center); // have to flip for 3.7


	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.wav", path_stream.str().c_str(),talkgroup,starttime,f);
	sprintf(status_filename, "%s/%ld-%ld_%g.json", path_stream.str().c_str(),talkgroup,starttime,freq);

	wav_sink->open(filename);

	active = true;
	valve->set_enabled(true);
}
