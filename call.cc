#include "call.h"

Call::Call(long t, double f) {
	talkgroup = t;
	freq = f;
	start_time = time(NULL);
	last_update = time(NULL);
	recording = false;
}

	void  Call::set_recorder(Recorder *r) {recorder = r;}
	Recorder *  Call::get_recorder() {return recorder;}
	double  Call::get_freq() {return freq;}
	void Call::set_freq(double f) {freq = f;} 
	long  Call::get_talkgroup() {return talkgroup;}
	void  Call::set_recording(bool m) {recording = m;}
	bool  Call::get_recording() { return recording;}
	void  Call::update() {last_update = time(NULL);}
	int  Call::since_last_update() {return time(NULL) - last_update;}
	long  Call::elapsed() {return time(NULL) - start_time;}