#ifndef ANALOG_RECORDER_H
#define ANALOG_RECORDER_H

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

#include <gnuradio/io_signature.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/iir_filter_ffd.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/filter/freq_xlating_fir_filter_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/rational_resampler_base_ccc.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/analog/sig_source_f.h>
#include <gnuradio/analog/sig_source_c.h>
//#include <gnuradio/analog/pwr_squelch_cc.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>
#include <gnuradio/block.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/copy.h>
//Valve
#include <gnuradio/blocks/head.h>

#include <gnuradio/blocks/wavfile_sink.h>
#include <gnuradio/blocks/file_sink.h>

#include "smartnet.h"
#include "recorder.h"


class analog_recorder;

typedef boost::shared_ptr<analog_recorder> analog_recorder_sptr;

analog_recorder_sptr make_analog_recorder(double f, double c, long s, long t, int n);

class analog_recorder : public gr::hier_block2, public Recorder
{
	friend analog_recorder_sptr make_analog_recorder(double f, double c, long s, long t, int n);
protected:
	analog_recorder(double f, double c, long s, long t, int n);

public:
	~analog_recorder();
	void tune_offset(double f);
	void activate(long talkgroup,double f, int num);

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
	bool muted;
	long talkgroup;
	long samp_rate;
	time_t timestamp;
	time_t starttime;
	char filename[160];
	char status_filename[160];
	char raw_filename[160];
	char debug_filename[160];
	int num;

	bool iam_logging;
	bool active;
	std::vector<float> lpf_taps;
	std::vector<float> resampler_taps;
	std::vector<float> audio_resampler_taps;
	std::vector<float> sym_taps;

	/* GR blocks */
	gr::filter::iir_filter_ffd::sptr deemph;
	gr::filter::fir_filter_ccf::sptr lpf;
	gr::filter::fir_filter_fff::sptr sym_filter;
	gr::filter::freq_xlating_fir_filter_ccf::sptr prefilter;
	gr::analog::sig_source_c::sptr offset_sig;

	gr::blocks::multiply_cc::sptr mixer;
	gr::blocks::file_sink::sptr fs;
	gr::blocks::multiply_const_ff::sptr quiet;
	gr::blocks::multiply_const_ff::sptr levels;


	gr::filter::rational_resampler_base_ccf::sptr downsample_sig;
	gr::filter::fir_filter_fff::sptr decim_audio;
	gr::filter::rational_resampler_base_fff::sptr upsample_audio;
	//gr::analog::pwr_squelch_cc::sptr squelch;
	gr::analog::quadrature_demod_cf::sptr demod;
	gr::blocks::wavfile_sink::sptr wav_sink;
	gr::blocks::file_sink::sptr raw_sink;
	gr::blocks::file_sink::sptr debug_sink;
	gr::blocks::null_sink::sptr null_sink;
	gr::blocks::head::sptr head_source;
	gr::blocks::copy::sptr valve;
	

};


#endif