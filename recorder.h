#ifndef RECORDER_H
#define RECORDER_H

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
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/filter/freq_xlating_fir_filter_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/rational_resampler_base_ccc.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include "dsd_block_ff.h"
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




class Recorder
{

public:
	virtual void tune_offset(double f) {};
	virtual void activate( long t, double f, int n) {};
	virtual void deactivate() {} ;
	virtual double get_freq() {return 0;};
	virtual long get_talkgroup() {return 0;};
	virtual bool is_active() {return false;};
	virtual char *get_filename() {return NULL;};
	/*
	private:
		double center, freq;
		long talkgroup;
	  long samp_rate;
		time_t timestamp;
		time_t starttime;
		char filename[160];
	  char status_filename[160];
		int num;

		bool iam_logging;
		bool active;*/
};


#endif
