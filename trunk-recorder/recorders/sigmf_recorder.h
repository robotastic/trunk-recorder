#ifndef SIGMF_RECORDER_H
#define SIGMF_RECORDER_H

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

#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/analog/feedforward_agc_cc.h>
#include <gnuradio/analog/agc2_ff.h>
#include <gnuradio/analog/agc2_cc.h>
#include <gnuradio/digital/diff_phasor_cc.h>

#include <gnuradio/blocks/complex_to_arg.h>


#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/blocks/multiply_const_cc.h>

#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>

#include <gnuradio/block.h>
#include <gnuradio/blocks/null_sink.h>

#include <gnuradio/blocks/copy.h>

#include <gnuradio/blocks/short_to_float.h>
#include <gnuradio/blocks/char_to_float.h>

#include <op25_repeater/fsk4_demod_ff.h>
#include <op25_repeater/fsk4_slicer_fb.h>
#include <op25_repeater/p25_frame_assembler.h>
#include <op25_repeater/gardner_costas_cc.h>
#include <op25_repeater/vocoder.h>
#include <gnuradio/msg_queue.h>
#include <gnuradio/message.h>
#include <gnuradio/blocks/head.h>
#include <gnuradio/blocks/file_sink.h>

#include "recorder.h"
#include "../config.h"
#include <gr_blocks/nonstop_wavfile_sink.h>
#include <gr_blocks/freq_xlating_fft_filter.h>


class Source;
class sigmf_recorder;
typedef boost::shared_ptr<sigmf_recorder> sigmf_recorder_sptr;
sigmf_recorder_sptr make_sigmf_recorder(Source *src);
#include "../source.h"

class sigmf_recorder : public gr::hier_block2, public Recorder
{
								friend sigmf_recorder_sptr make_sigmf_recorder(Source *src);
protected:
								sigmf_recorder(Source *src);

public:
								~sigmf_recorder();

								void tune_offset(double f);
								void start( Call *call);
								void stop();
								double get_freq();
								int get_num();
								double get_current_length();
								bool is_active();
								State get_state();
								int lastupdate();
								long elapsed();
								Source *get_source();
								long get_source_count();
								Call_Source *get_source_list();

								gr::msg_queue::sptr tune_queue;
								gr::msg_queue::sptr traffic_queue;
								gr::msg_queue::sptr rx_queue;
								//void forecast(int noutput_items, gr_vector_int &ninput_items_required);

private:
								double center, freq;
								bool qpsk_mod;
								int silence_frames;
								long talkgroup;
								time_t timestamp;
								time_t starttime;

								Config *config;
								Source *source;
								char filename[160];
								//int num;
								State state;


								//std::vector<gr_complex> lpf_coeffs;
								std::vector<float> lpf_coeffs;
								std::vector<float> arb_taps;
								std::vector<float> sym_taps;

								gr::filter::freq_xlating_fir_filter_ccf::sptr prefilter;

								/* GR blocks */
								gr::filter::fir_filter_ccf::sptr lpf;
								gr::filter::fir_filter_fff::sptr sym_filter;

								gr::analog::sig_source_c::sptr lo;

								gr::digital::diff_phasor_cc::sptr diffdec;

								gr::blocks::multiply_cc::sptr mixer;
								gr::blocks::file_sink::sptr fs;

								gr::filter::pfb_arb_resampler_ccf::sptr arb_resampler;
								gr::filter::rational_resampler_base_ccf::sptr downsample_sig;
								gr::filter::rational_resampler_base_fff::sptr upsample_audio;
								gr::analog::quadrature_demod_cf::sptr fm_demod;
								gr::analog::feedforward_agc_cc::sptr agc;
								gr::analog::agc2_ff::sptr demod_agc;
								gr::analog::agc2_cc::sptr pre_demod_agc;
								gr::blocks::nonstop_wavfile_sink::sptr wav_sink;
								gr::blocks::file_sink::sptr raw_sink;
								gr::blocks::short_to_float::sptr converter;
								gr::blocks::copy::sptr valve;

								gr::blocks::multiply_const_ff::sptr multiplier;
								gr::blocks::multiply_const_ff::sptr rescale;
								gr::blocks::multiply_const_ff::sptr baseband_amp;
								gr::blocks::complex_to_arg::sptr to_float;
								gr::op25_repeater::fsk4_demod_ff::sptr fsk4_demod;
								gr::op25_repeater::p25_frame_assembler::sptr op25_frame_assembler;

								gr::op25_repeater::fsk4_slicer_fb::sptr slicer;
								gr::op25_repeater::vocoder::sptr op25_vocoder;
								gr::op25_repeater::gardner_costas_cc::sptr costas_clock;
};


#endif
