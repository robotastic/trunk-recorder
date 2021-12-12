#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "../recorders/recorder.h"
#include "../systems/system.h"
#include "../source.h"
#include "../call_concluder/call_concluder.h"

#include "plugin_api.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/optional/optional.hpp>
#include <vector>
#include <stdlib.h>


typedef boost::shared_ptr<Plugin_Api> (pluginapi_create_t)();



struct Plugin {
boost::function<pluginapi_create_t> creator;
boost::shared_ptr<Plugin_Api> api;
plugin_state_t state;
std::string name;
};


void initialize_plugins(boost::property_tree::ptree &cfg, Config* config, std::vector<Source *> sources, std::vector<System *> systems);
void add_internal_plugin(std::string name, std::string library, boost::property_tree::ptree &cfg);
void start_plugins(std::vector<Source *> sources, std::vector<System *> systems);
void stop_plugins();

void plugman_poll_one();
void plugman_audio_callback(Recorder *recorder, int16_t *samples, int sampleCount);
int plugman_signal(long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder);
int plugman_call_start(Call *call);
int plugman_call_end(Call_Data_t call_info);
int plugman_calls_active(std::vector<Call *> calls);
void plugman_setup_recorder(Recorder *recorder);
void plugman_setup_system(System * system);
void plugman_setup_systems(std::vector<System *> systems);
void plugman_setup_sources(std::vector<Source *> sources);
void plugman_setup_config(std::vector<Source *> sources, std::vector<System *> systems);
void plugman_system_rates(std::vector<System *> systems, float timeDiff);
void plugman_unit_registration(System * system, long source_id);
void plugman_unit_deregistration(System * system, long source_id);
void plugman_unit_acknowledge_response(System * system, long source_id);
void plugman_unit_group_affiliation(System * system, long source_id, long talkgroup_num);
#endif // PLUGIN_MANAGER_H