#ifndef RECORDER_GLOBALS_H
#define RECORDER_GLOBALS_H

#include "systems/system.h"
#include "recorders/recorder.h"
#include "../lib/gr_blocks/decoder_wrapper.h"

void process_signal(long unitId, const char* signaling_type, gr::blocks::SignalType sig_type, System* system, Recorder* recorder);

#endif /* RECORDER_GLOBALS_H */