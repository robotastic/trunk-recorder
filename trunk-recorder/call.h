#ifndef CALL_H
#define CALL_H
#include <sys/time.h>
#include <boost/log/trivial.hpp>

struct Call_Source {
								long source;
								long time;
								double position;

};

struct Call_Freq {
	double freq;
	long time;
	double position;
	double total_len;
	double error_count;
	double spike_count;
};

struct Call_Error {
	double freq;
	double sample_count;
	double error_count;
	double spike_count;
};

class Recorder;
#include <op25_repeater/include/op25_repeater/rx_status.h>
#include "uploaders/call_uploader.h"
#include "config.h"
#include "state.h"
#include "systems/system.h"
#include "systems/parser.h"
#include <string>


class System;
//enum  CallState { monitoring=0, recording=1, stopping=2};

class Call {
public:

								Call( long t, double f, System *s, Config c);
								Call( TrunkMessage message, System *s, Config c);
								virtual ~Call();
								virtual void restart_call();
								void end_call();

								void set_sigmf_recorder(Recorder *r);
								Recorder * get_sigmf_recorder();
								void set_debug_recorder(Recorder *r);
								Recorder * get_debug_recorder();
								virtual void set_recorder(Recorder *r);
								Recorder * get_recorder();
								double get_freq();
								char * get_status_filename();
								char *get_converted_filename();
								char *get_filename();
								char * get_debug_filename();
								char * get_sigmf_filename();
								int get_sys_num();
								std::string get_short_name();
								void create_filename();
								void set_error(Rx_Status rx_status);
								void set_freq(double f);
								long get_talkgroup();
								long get_source_count();
								Call_Source *get_source_list();
								Call_Freq *get_freq_list();
								Call_Error *get_error_list();
								long get_error_list_count();
								long get_freq_count();
								void update(TrunkMessage message);
								int get_idle_count();
								void increase_idle_count();
								void reset_idle_count();
								int since_last_update();
								long stopping_elapsed();
								long elapsed();
								long get_start_time();
								double get_current_length();
								long get_stop_time();
								void set_debug_recording(bool m);
								bool get_debug_recording();
								void set_sigmf_recording(bool m);
								bool get_sigmf_recording();
								void set_state(State s);
								State get_state();
								void set_phase2_tdma(bool m);
								bool get_phase2_tdma();
								void set_tdma_slot(int s);
								int get_tdma_slot();
								const char * get_xor_mask();
								//virtual bool is_conventional() { return true;}
								virtual bool is_conventional() { return false;}
								void set_encrypted(bool m);
								bool get_encrypted();
								void set_emergency(bool m);
								bool get_emergency();
								std::string get_talkgroup_display();
								void set_talkgroup_display_format(std::string format);
								void set_talkgroup_tag(std::string tag);
								boost::property_tree::ptree get_stats();
								
								std::string get_talkgroup_tag();
								double get_final_length();
protected:
								State state;
								long talkgroup;
								double curr_freq;
								System *sys;
								std::string short_name;
								int src_count;
								long curr_src_id;
								Call_Error error_list[50];
								Call_Source src_list[50];
								Call_Freq freq_list[50];
								long error_list_count;
								long freq_count;
								time_t last_update;
								int idle_count;
								time_t stop_time;
								time_t start_time;
								bool debug_recording;
								bool sigmf_recording;
								bool encrypted;
								bool emergency;
								char filename[255];
								char converted_filename[255];
								char status_filename[255];
								char debug_filename[255];
								char sigmf_filename[255];
								bool phase2_tdma;
								int tdma_slot;
								double _final_length;

								Config config;
								Recorder *recorder;
								Recorder *debug_recorder;
								Recorder *sigmf_recorder;
								bool add_source(long src);
								std::string talkgroup_display;
								std::string talkgroup_tag;
								void update_talkgroup_display();

};

#endif
