#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "../recorders/recorder.h"
#include "../systems/system.h"
#include "../source.h"
#include <boost/property_tree/ptree.hpp>
#include <vector>
#include <stdlib.h>

void initialize_plugins(boost::property_tree::ptree &cfg);
void start_plugins(std::vector<Source *> sources, std::vector<System *> systems);
void stop_plugins();

void plugman_poll_one();
void plugman_audio_callback(Recorder *recorder, float *samples, int sampleCount);
void plugman_signal(long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder);
void plugman_call_start(Call *call);
void plugman_call_end(Call *call);
void plugman_setup_recorder(Recorder *recorder);
void plugman_setup_system(System * system);
void plugman_setup_systems(std::vector<System *> systems);
void plugman_setup_sources(std::vector<Source *> sources);

#endif // PLUGIN_MANAGER_H