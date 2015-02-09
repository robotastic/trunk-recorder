#ifndef CALL_H
#define CALL_H
#include <sys/time.h>

#include "parser.h"
#include "recorder.h"

class Call {
	long talkgroup;
	double freq;
	time_t last_update;
	time_t start_time;
	bool recording;
	bool encrypted;
	bool emergency;
	int tdma;
	long source;
	Recorder *recorder;
public:
	Call( long t, double f);
	Call( TrunkMessage message );
	void set_recorder(Recorder *r);
	Recorder * get_recorder();
	double get_freq();
	void set_freq(double f);
	long get_talkgroup();
	long get_source();
	void set_source(long s);
	void update();
	int since_last_update();
	long elapsed();
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