#ifndef SOURCE_H
#define SOURCE_H
#include <iostream>
#include <gnuradio/basic_block.h>
#include <gnuradio/top_block.h>
#include <osmosdr/source.h>
#include <gnuradio/uhd/usrp_source.h>

#define DSD

#include "recorder.h"
#include "analog_recorder.h"
#include "dsd_recorder.h"
#include "debug_recorder.h"
#include "p25_recorder.h"

class Source
{
	double min_hz;
	double max_hz;
	double center;
	double rate;
	double error;
	int gain;
	int bb_gain;
	int if_gain;
	int max_digital_recorders;
	int max_debug_recorders;
	int max_analog_recorders;

#ifdef DSD
	std::vector<dsd_recorder_sptr> digital_recorders;
#else
	std::vector<p25_recorder_sptr> digital_recorders;
#endif
	std::vector<debug_recorder_sptr> debug_recorders;
	std::vector<analog_recorder_sptr> analog_recorders;
	std::string driver;
	std::string device;
	std::string antenna;
	gr::basic_block_sptr source_block;

public:
	int get_num_available_recorders();
	Source(double c, double r, double e, std::string driver, std::string device);
	gr::basic_block_sptr get_src_block();
	double get_min_hz();
	double get_max_hz();
	double get_center();
	double get_rate();
	std::string get_driver();
	std::string get_device();
	void set_antenna(std::string ant);
	std::string get_antenna();
	void set_error(double e);
	double get_error();
	void set_if_gain(int i);
	int get_if_gain();
	void set_gain(int r);
	int get_gain();
	void set_bb_gain(int b);
	int get_bb_gain();
	void create_analog_recorders(gr::top_block_sptr tb, int r);
	Recorder * get_analog_recorder(int priority);
	void create_digital_recorders(gr::top_block_sptr tb, int r);
	Recorder * get_digital_recorder(int priority);
	void create_debug_recorders(gr::top_block_sptr tb, int r);
	Recorder * get_debug_recorder();
	inline osmosdr::source::sptr cast_to_osmo_sptr(gr::basic_block_sptr p)
	{
		return boost::dynamic_pointer_cast<osmosdr::source, gr::basic_block>(p);
	}

	inline gr::uhd::usrp_source::sptr cast_to_usrp_sptr(gr::basic_block_sptr p)
	{
		return boost::dynamic_pointer_cast<gr::uhd::usrp_source, gr::basic_block>(p);
	}
};
#endif