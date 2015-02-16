#ifndef P25_TRUNKING_H
#define P25_TRUNKING_H

#define _USE_MATH_DEFINES

#include <cstdio>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <time.h>


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
//#include <gnuradio/analog/squelch_base_cc.h>
//#include <gnuradio/analog/pwr_squelch_cc.h>

#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>
#include <gnuradio/block.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/blocks/copy.h>

#include <gnuradio/blocks/short_to_float.h>
#include <gnuradio/blocks/char_to_float.h>
#include <op25/decoder_bf.h>
#include <op25/fsk4_demod_ff.h>
#include <op25/fsk4_slicer_fb.h>
#include <op25_repeater/fsk4_slicer_fb.h>
#include <op25_repeater/p25_frame_assembler.h>
#include <op25_repeater/gardner_costas_cc.h>
#include <op25_repeater/vocoder.h>

#include <gnuradio/msg_queue.h>
#include <gnuradio/message.h>

#include <gnuradio/blocks/head.h>

#include <gnuradio/blocks/wavfile_sink.h>
#include <gnuradio/blocks/file_sink.h>
#include "recorder.h"
#include "smartnet.h"


class p25_trunking;

typedef boost::shared_ptr<p25_trunking> p25_trunking_sptr;

p25_trunking_sptr make_p25_trunking(double f, double c, long s, gr::msg_queue::sptr queue);

class p25_trunking : public gr::hier_block2, public Recorder
{
	friend p25_trunking_sptr make_p25_trunking(double f, double c, long s, gr::msg_queue::sptr queue);
protected:
	p25_trunking(double f, double c, long s, gr::msg_queue::sptr queue);

public:
	~p25_trunking();

	void tune_offset(double f);
	void activate( long talkgroup, double f, int num);

	void deactivate();
	double get_freq();
	bool is_active();
	int lastupdate();
	long elapsed();
	void mute();
	void unmute();
	char *get_filename();
	gr::msg_queue::sptr tune_queue;
	gr::msg_queue::sptr traffic_queue;
	gr::msg_queue::sptr rx_queue;
	//void forecast(int noutput_items, gr_vector_int &ninput_items_required);

private:
	double center, freq;
	bool muted;
	long talkgroup;
	time_t timestamp;
	time_t starttime;

	char filename[160];
	char raw_filename[160];
	int num;

	bool iam_logging;
	bool active;


	std::vector<float> lpf_taps;
	std::vector<float> resampler_taps;
	std::vector<float> sym_taps;

	/* GR blocks */
	gr::filter::fir_filter_ccf::sptr lpf;
	gr::filter::fir_filter_fff::sptr sym_filter;
	gr::filter::freq_xlating_fir_filter_ccf::sptr prefilter;
	gr::analog::sig_source_c::sptr offset_sig;


	gr::blocks::multiply_cc::sptr mixer;
	gr::blocks::file_sink::sptr fs;



	gr::filter::rational_resampler_base_ccf::sptr downsample_sig;
	gr::filter::rational_resampler_base_fff::sptr upsample_audio;
	//gr::analog::quadrature_demod_cf::sptr demod;
	gr::analog::quadrature_demod_cf::sptr demod;
	gr::blocks::wavfile_sink::sptr wav_sink;
	gr::blocks::file_sink::sptr raw_sink;
	gr::blocks::null_sink::sptr null_sink;
	gr::blocks::null_sink::sptr dump_sink;
	gr::blocks::head::sptr head_source;
	gr::blocks::short_to_float::sptr converter;
	gr::blocks::copy::sptr valve;
	//gr::blocks::char_to_float::sptr converter;
	gr::blocks::multiply_const_ff::sptr multiplier;
	//gr::analog::pwr_squelch_cc::sptr squelch;
	gr::op25::fsk4_demod_ff::sptr op25_demod;
	gr::op25_repeater::p25_frame_assembler::sptr op25_frame_assembler;

	gr::op25_repeater::fsk4_slicer_fb::sptr op25_slicer;
	gr::op25_repeater::vocoder::sptr op25_vocoder;

	unsigned GCD(unsigned u, unsigned v);
	std::vector<float> design_filter(double interpolation, double deci);
};


#endif

