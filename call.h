#ifndef CALL_H
#define CALL_H
#include <sys/time.h>

#include "recorder.h"

class Call {
	long talkgroup;
	double freq;
	time_t last_update;
	time_t start_time;
	bool recording;
	Recorder *recorder;
public:
	Call( long t, double f);
	void set_recorder(Recorder *r);
	Recorder * get_recorder();
	double get_freq();
	void set_freq(double f);
	long get_talkgroup();
	void update();
	int since_last_update();
	long elapsed();
	void set_recording(bool m);
	bool get_recording();
};
#endif