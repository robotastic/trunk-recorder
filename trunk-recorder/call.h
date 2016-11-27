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
};

class Recorder;

#include "uploader.h"
#include "config.h"
#include "state.h"
#include "recorders/recorder.h"
#include "systems/system.h"
#include "systems/parser.h"

//enum  CallState { monitoring=0, recording=1, stopping=2};

class Call {
public:

								Call( long t, double f, System *s, Config c);
								Call( TrunkMessage message, System *s, Config c);
								~Call();
								void end_call();
								void set_debug_recorder(Recorder *r);
								Recorder * get_debug_recorder();
								void set_recorder(Recorder *r);
								Recorder * get_recorder();
								double get_freq();

								char *get_converted_filename();
								char *get_filename();
								void create_filename();
								void set_freq(double f);
								long get_talkgroup();
								long get_source_count();
								Call_Source *get_source_list();
								Call_Freq *get_freq_list();
								long get_freq_count();
								void update(TrunkMessage message);
								int since_last_update();
								long stopping_elapsed();
								long elapsed();
								long get_start_time();

								long get_stop_time();
								void set_debug_recording(bool m);
								bool get_debug_recording();
								void set_state(State s);
								State get_state();
								void set_tdma(int m);
								int get_tdma();
								void set_encrypted(bool m);
								bool get_encrypted();
								void set_emergency(bool m);
								bool get_emergency();
private:
								State state;
								long talkgroup;
								double curr_freq;
								System *sys;
								Call_Freq freq_list[50];
								long freq_count;
								time_t last_update;

								time_t stop_time;
								time_t start_time;
								bool debug_recording;
								bool encrypted;
								bool emergency;
								char filename[160];
								char converted_filename[160];
								char status_filename[160];
								int tdma;

								Config config;
								Recorder *recorder;
								Recorder *debug_recorder;
};

#endif
