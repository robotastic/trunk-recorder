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
#include <gnuradio/analog/sig_source_f.h>
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>
#include <gnuradio/block.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/copy.h>

#include <gnuradio/blocks/head.h>

#include <gnuradio/blocks/file_sink.h>

#include <op25_repeater/include/op25_repeater/rx_status.h>
#include <gr_blocks/nonstop_wavfile_sink.h>

#include "../state.h"


unsigned GCD(unsigned u, unsigned v);
std::vector<float> design_filter(double interpolation, double deci);


class Recorder
{

public:
	Recorder(std::string type);
	virtual void tune_offset(double f) {};
	virtual void start( Call *call) {};
	virtual void stop() {};
  	virtual void set_tdma_slot(int slot) {};
	virtual double get_freq() {return 0;};
  	virtual Source *get_source() {return NULL;};
	virtual Call_Source *get_source_list() {return NULL;};
	virtual int get_num() {return -1;};
	virtual long get_source_count() {return 0;};
	virtual long get_talkgroup() {return 0;};
	virtual State get_state() {return inactive;};
	virtual Rx_Status get_rx_status() {Rx_Status rx_status={0,0,0}; return rx_status; }
	virtual bool is_active() {return false;};
	virtual bool is_analog() {return false;};
	virtual bool is_idle() {return true;};
	virtual double get_current_length(){return 0;};
	virtual void clear(){};
	int rec_num;
	static int rec_counter;
	virtual boost::property_tree::ptree get_stats();

protected:
	int 	recording_count;
	double	recording_duration;
	std::string type;
};


#endif
