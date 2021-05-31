#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "../recorders/recorder.h"
#include "../systems/system.h"
#include "../source.h"
#include "../call_concluder/call_concluder.h"

#include "plugin_api.h"
#include <boost/property_tree/ptree.hpp>
#include <vector>
#include <stdlib.h>


typedef boost::shared_ptr<Plugin_Api> (pluginapi_create_t)();

struct Plugin {
boost::function<pluginapi_create_t> creator;
boost::shared_ptr<Plugin_Api> api;
plugin_state_t state;
};


void initialize_plugins(boost::property_tree::ptree &cfg, Config* config, std::vector<Source *> sources, std::vector<System *> systems);
void initialize_internal_plugin(std::string name); // For use by internal plugins, like the uploaders. Should be called before initialize_plugins
void start_plugins(std::vector<Source *> sources, std::vector<System *> systems);
void stop_plugins();

void plugman_poll_one();
void plugman_audio_callback(Recorder *recorder, float *samples, int sampleCount);
void plugman_signal(long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder);
void plugman_call_start(Call *call);
void plugman_call_end(Call_Data_t call_info);
void plugman_calls_active(std::vector<Call *> calls);
void plugman_setup_recorder(Recorder *recorder);
void plugman_setup_system(System * system);
void plugman_setup_systems(std::vector<System *> systems);
void plugman_setup_sources(std::vector<Source *> sources);
void plugman_setup_config(std::vector<Source *> sources, std::vector<System *> systems);
void plugman_system_rates(std::vector<System *> systems, float timeDiff);

#endif // PLUGIN_MANAGER_H