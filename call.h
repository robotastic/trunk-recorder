#ifndef CALL_H
#define CALL_H
#include <sys/time.h>
#include <boost/log/trivial.hpp>

class Recorder;
#include "parser.h"
#include "recorder.h"

class Call {
	long talkgroup;
	double freq;
	time_t last_update;
	time_t start_time;
	bool recording;
	bool debug_recording;
	bool encrypted;
	bool emergency;
    char filename[160];
	int tdma;
    long src_count;
    long src_list[50];
	Recorder *recorder;
	Recorder *debug_recorder;
public:
	Call( long t, double f);
	Call( TrunkMessage message );
    void end_call();
	void set_debug_recorder(Recorder *r);
	Recorder * get_debug_recorder();
	void set_recorder(Recorder *r);
	Recorder * get_recorder();
	double get_freq();
    char *get_filename();
    void create_filename(); 
	void set_freq(double f);
	long get_talkgroup();
    long get_source_count(); 
    long *get_source_list();
    bool add_source(long src);
	void update(TrunkMessage message);
	int since_last_update();
	long elapsed();
	void set_debug_recording(bool m);
	bool get_debug_recording();
	void set_recording(bool m);
	bool get_recording();
	void set_tdma(int m);
	int get_tdma();
	void set_encrypted(bool m);
	bool get_encrypted();
	void set_emergency(bool m);
	bool get_emergency();
};
#endif