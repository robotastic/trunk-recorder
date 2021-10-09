#ifndef STREAMER_PROTO_HELPER_H
#define STREAMER_PROTO_HELPER_H

#include "streamer.pb.h"
#include "../../call.h"
#include "../../recorders/recorder.h"
#include "../../systems/system.h"
#include "../../source.h"

streamer::AudioSample* ToAudioSample(Recorder *recorder, float *samples, int sampleCount);
streamer::CallInfo* ToCallInfo(Call *call);
streamer::SystemInfo* ToSystemInfo(System *system);
streamer::SourceInfo* ToSourceInfo(Source *source);
streamer::RecorderInfo* ToRecorderInfo(Recorder *recorder);
streamer::SignalInfo* ToSignalInfo(long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder);

#endif // STREAMER_PROTO_HELPER_H