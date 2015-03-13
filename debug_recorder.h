#ifndef DEBUG_RECORDER_H
#define DEBUG_RECORDER_H

#include <cstdio>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <fstream>


#include <boost/shared_ptr.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>

#include <gnuradio/io_signature.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/filter/freq_xlating_fir_filter_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/rational_resampler_base_ccc.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/analog/sig_source_f.h>
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>
#include <gnuradio/block.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/copy.h>

//Valve
//#include <gnuradio/blocks/null_sink.h>
//#include <gnuradio/blocks/null_source.h>
#include <gnuradio/blocks/head.h>
//#include <gnuradio/kludge_copy.h>
//#include <smartnet_wavsink.h>
#include "nonstop_wavfile_sink.h"

//#include <gnuradio/blocks/wavfile_sink.h>
#include <gnuradio/blocks/file_sink.h>
//#include <blocks/wavfile_sink.h>
#include "recorder.h"
#include "smartnet.h"


class debug_recorder;

typedef boost::shared_ptr<debug_recorder> debug_recorder_sptr;

debug_recorder_sptr make_debug_recorder(float f, float c, long s, long t, int n);

class debug_recorder : public gr::hier_block2 , public Recorder
{
	friend debug_recorder_sptr make_debug_recorder(float f, float c, long s, long t, int n);
protected:
	debug_recorder(double f, double c, long s, long t, int n);

public:
	~debug_recorder();
	void tune_offset(double f);
	void activate( long t, double f, int n);

	void deactivate();
	double get_freq();
	long get_talkgroup();
	bool is_active();
	int lastupdate();
	long elapsed();
	void close();
	void mute();
	void unmute();
	char *get_filename();
	//void forecast(int noutput_items, gr_vector_int &ninput_items_required);
	static bool logging;
private:
	double center, freq;
	bool recording;
	long talkgroup;
	long samp_rate;
	time_t timestamp;
	time_t starttime;
	char filename[160];

	int num;

	bool iam_logging;
	bool active;
	std::vector<float> lpf_taps;
	std::vector<float> resampler_taps;
	std::vector<float> sym_taps;

	/* GR blocks */
	gr::filter::fir_filter_ccf::sptr lpf;
	gr::filter::fir_filter_fff::sptr lpf_second;
	gr::filter::fir_filter_fff::sptr sym_filter;
	gr::filter::freq_xlating_fir_filter_ccf::sptr prefilter;
	gr::analog::sig_source_c::sptr offset_sig;

	gr::blocks::multiply_cc::sptr mixer;
	gr::blocks::file_sink::sptr fs;
	gr::blocks::multiply_const_ff::sptr quiet;
	gr::blocks::multiply_const_ff::sptr levels;


	gr::filter::rational_resampler_base_ccf::sptr downsample_sig;
	gr::filter::rational_resampler_base_fff::sptr upsample_audio;
	//gr::analog::quadrature_demod_cf::sptr demod;
	gr::analog::quadrature_demod_cf::sptr demod;

	gr::blocks::file_sink::sptr raw_sink;
	gr::blocks::null_sink::sptr null_sink;
	gr::blocks::head::sptr head_source;
	gr::blocks::copy::sptr valve;
	//gr_kludge_copy_sptr copier;

};


#endif
