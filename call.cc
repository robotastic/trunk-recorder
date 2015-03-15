#include "call.h"

Call::Call(long t, double f) {
	talkgroup = t;
	freq = f;
	start_time = time(NULL);
	last_update = time(NULL);
	recording = false;
	debug_recording = false;
	tdma = false;
	encrypted = false;
	emergency = false;
	source = 0;

}

Call::Call(TrunkMessage message) {
	talkgroup = message.talkgroup;
	freq = message.freq;
	start_time = time(NULL);
	last_update = time(NULL);
	recording = false;
	debug_recording = false;
	tdma = message.tdma;
	encrypted = message.encrypted;
	emergency = message.emergency;
	source = 0;

}

void  Call::set_debug_recorder(Recorder *r) {
	debug_recorder = r;
}

Recorder *  Call::get_debug_recorder() {
	return debug_recorder;
}

void  Call::set_recorder(Recorder *r) {
	recorder = r;
}
Recorder *  Call::get_recorder() {
	return recorder;
}
double  Call::get_freq() {
	return freq;
}
void Call::set_freq(double f) {
	freq = f;
}
long  Call::get_talkgroup() {
	return talkgroup;
}
void  Call::set_source(long s) {
	source = s;
}
long  Call::get_source() {
	return source;
}
void  Call::set_debug_recording(bool m) {
	debug_recording = m;
}
bool  Call::get_debug_recording() {
	return debug_recording;
}
void  Call::set_recording(bool m) {
	recording = m;
}
bool  Call::get_recording() {
	return recording;
}
void  Call::set_encrypted(bool m) {
	encrypted = m;
}
bool  Call::get_encrypted() {
	return encrypted;
}
void  Call::set_emergency(bool m) {
	emergency = m;
}
bool  Call::get_emergency() {
	return emergency;
}
void  Call::set_tdma(int m) {
	tdma = m;
}
int  Call::get_tdma() {
	return tdma;
}
void  Call::update() {
	last_update = time(NULL);
}
int  Call::since_last_update() {
	return time(NULL) - last_update;
}
long  Call::elapsed() {
	return time(NULL) - start_time;
}