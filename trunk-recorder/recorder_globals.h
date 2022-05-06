#ifndef RECORDER_GLOBALS_H
#define RECORDER_GLOBALS_H

#include "gr_blocks/decoder_wrapper.h"

class Call;
class Recorder;
class System;
void process_signal(long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder);

#endif /* RECORDER_GLOBALS_H */