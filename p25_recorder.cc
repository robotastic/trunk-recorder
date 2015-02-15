
#include "p25_recorder.h"


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
        double symbol_deviation = 600.0;
		double prechannel_decim = floor(capture_rate / system_channel_rate);
        double prechannel_rate = capture_rate / prechannel_decim;
        double trans_width = 12500 / 2;
        double trans_centre = trans_width + (trans_width / 2);

	std::vector<float> sym_taps;
	const double pi = M_PI; //boost::math::constants::pi<double>();

	timestamp = time(NULL);
	starttime = time(NULL);



	prefilter = gr::filter::freq_xlating_fir_filter_ccf::make(int(prechannel_decim),
		gr::filter::firdes::low_pass(1.0, capture_rate, trans_centre,trans_width, gr::filter::firdes::WIN_HANN),  //trans_centre, trans_width, gr::filter::firdes::WIN_HANN),
		offset, 
		capture_rate);

	//int squelch_db = 40;
	// squelch = gr::analog::pwr_squelch_cc::make(squelch_db, 0.001, 0, true);
std::cout << "Prechannel Decim: " << floor(capture_rate / system_channel_rate) << " Rate: " << prechannel_rate << " system_channel_rate: " << system_channel_rate << std::endl;
	
		unsigned int d = GCD(prechannel_rate, system_channel_rate);
    	double small_system_channel_rate = floor(system_channel_rate  / d);
    	double small_prechannel_rate = floor(prechannel_rate / d);
std::cout << "After GCD - Prechannel Decim: " << prechannel_decim << " Rate: " << small_prechannel_rate << " system_channel_rate: " << small_system_channel_rate << std::endl;


	resampler_taps = design_filter(small_system_channel_rate, small_prechannel_rate);

	downsample_sig = gr::filter::rational_resampler_base_ccf::make(small_system_channel_rate, small_prechannel_rate, resampler_taps);
	//resampler_taps = design_filter(small_prechannel_rate, small_system_channel_rate);

	//downsample_sig = gr::filter::pfb_arb_resampler_ccf::make(float(system_channel_rate) / float(prechannel_rate));

	
	double fm_demod_gain = floor(system_channel_rate / (2.0 * pi * symbol_deviation));
	demod = gr::analog::quadrature_demod_cf::make(fm_demod_gain);

	double symbol_decim = 1;

	valve = gr::blocks::copy::make(sizeof(gr_complex));
	valve->set_enabled(false);

	std::cout << " FM Gain: " << fm_demod_gain << " PI: " << pi << " Samples per sym: " << samples_per_symbol <<  std::endl;
	
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
	op25_demod = gr::op25::fsk4_demod_ff::make(tune_queue, system_channel_rate, symbol_rate);
	op25_slicer = gr::op25_repeater::fsk4_slicer_fb::make(levels);

	int udp_port = 0;
	int verbosity = 10;
	const char * wireshark_host="127.0.0.1";
	bool do_imbe = 1;
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
	wav_sink = gr::blocks::wavfile_sink::make(filename,1,8000,16);
	null_sink = gr::blocks::null_sink::make(sizeof(gr_complex));


		
	connect(self(),0, null_sink,0);
	/*
	connect(self(),0, valve,0);
	connect(valve,0, prefilter,0);
	connect(prefilter, 0, downsample_sig, 0);
	connect(downsample_sig, 0, demod, 0);
	connect(demod, 0, sym_filter, 0);
	connect(sym_filter, 0, op25_demod, 0);
	connect(op25_demod,0, op25_slicer, 0);
	connect(op25_slicer,0, op25_frame_assembler,0);
	connect(op25_frame_assembler, 0,  converter,0);
    connect(converter, 0, multiplier,0);
    connect(multiplier, 0, wav_sink,0);*/

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
	prefilter->set_center_freq(offset_amount); // have to flip this for 3.7
	//std::cout << "Offset set to: " << offset_amount << " Freq: "  << freq << std::endl;
}

void p25_recorder::deactivate() {
	std::cout<< "logging_receiver_dsd.cc: Deactivating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ] " << std::endl; 

	active = false;
	//valve->set_enabled(false);
	wav_sink->close();
	lock();
	connect(self(),0,null_sink,0);
	disconnect(self(),0, prefilter,0);
	disconnect(prefilter, 0, downsample_sig, 0);
	disconnect(downsample_sig, 0, demod, 0);
	disconnect(demod, 0, sym_filter, 0);
	disconnect(sym_filter, 0, op25_demod, 0);
	disconnect(op25_demod,0, op25_slicer, 0);
	disconnect(op25_slicer,0, op25_frame_assembler,0);
	disconnect(op25_frame_assembler, 0,  converter,0);
    disconnect(converter, 0, multiplier,0);
    disconnect(multiplier, 0, wav_sink,0);
    unlock();

}

void p25_recorder::activate(long t, double f, int n) {

	timestamp = time(NULL);
	starttime = time(NULL);

	talkgroup = t;
	freq = f;

  	tm *ltm = localtime(&starttime);
  	std::cout<< "logging_receiver_dsd.cc: Activating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]  "  <<std::endl;

	prefilter->set_center_freq(f - center); // have to flip for 3.7



	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.wav", path_stream.str().c_str(),talkgroup,starttime,f);
	


	wav_sink->open(filename);
	lock();
	disconnect(self(),0,null_sink,0);
	connect(self(),0, prefilter,0);
	connect(prefilter, 0, downsample_sig, 0);
	connect(downsample_sig, 0, demod, 0);
	connect(demod, 0, sym_filter, 0);
	connect(sym_filter, 0, op25_demod, 0);
	connect(op25_demod,0, op25_slicer, 0);
	connect(op25_slicer,0, op25_frame_assembler,0);
	connect(op25_frame_assembler, 0,  converter,0);
    connect(converter, 0, multiplier,0);
    connect(multiplier, 0, wav_sink,0);
    unlock();

	active = true;
	//valve->set_enabled(true);

}

	
