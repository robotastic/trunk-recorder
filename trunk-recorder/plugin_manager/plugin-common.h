#ifndef PLUGIN_COMMON_H
#define PLUGIN_COMMON_H

#include <pthread.h>
#include <stdlib.h>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include "../recorders/recorder.h"
#include "../systems/system.h"
#include "../call_concluder/call_concluder.h"
#include "../source.h"
#include "../global_structs.h"

#ifndef __MINGW32__
#define MODULE_EXPORT extern "C"
#else
#define MODULE_EXPORT extern "C" __declspec(dllexport)
#endif

typedef enum {
    PLUGIN_UNKNOWN = 0,
    PLUGIN_INITIALIZED,
    PLUGIN_RUNNING,
    PLUGIN_FAILED,
    PLUGIN_STOPPED,
    PLUGIN_DISABLED
} plugin_state_t;
#define PLUGIN_STATE_CNT 6

typedef struct plugin_t plugin_t;

struct plugin_t {
    void *plugin_data; // This is internal data for use by the plugin.
    plugin_state_t state;
    int (*init)(plugin_t * const plugin, Config* config);
    int (*parse_config)(plugin_t * const plugin, boost::property_tree::ptree::value_type &cfg);
    int (*start)(plugin_t * const plugin);
    int (*stop)(plugin_t * const plugin);
    int (*poll_one)(plugin_t * const plugin);
    int (*signal)(plugin_t * const plugin, long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder);
    int (*audio_stream)(plugin_t * const plugin, Recorder *recorder, int16_t *samples, int sampleCount);
    int (*call_start)(plugin_t * const plugin, Call *call);
    int (*call_end)(plugin_t * const plugin, Call_Data_t call_info);
    int (*calls_active)(plugin_t * const plugin, std::vector<Call *> calls);
    int (*setup_recorder)(plugin_t * const plugin, Recorder *recorder);
    int (*setup_system)(plugin_t * const plugin, System * system);
    int (*setup_systems)(plugin_t * const plugin, std::vector<System *> systems);
    int (*setup_sources)(plugin_t * const plugin, std::vector<Source *> sources);
    int (*setup_config)(plugin_t * const plugin, std::vector<Source *> sources, std::vector<System *> systems);
    int (*system_rates)(plugin_t * const plugin, std::vector<System *> systems, float timeDiff);
};

plugin_t *plugin_new(const char * const plugin_file, char const * const plugin_name);
int plugin_init(plugin_t * const plugin, Config* config);
int plugin_parse_config(plugin_t * const plugin, boost::property_tree::ptree::value_type &cfg);
int plugin_start(plugin_t * const plugin);
int plugin_stop(plugin_t * const plugin);
int plugin_poll_one(plugin_t * const plugin);
int plugin_signal(plugin_t * const plugin, long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder);
int plugin_audio_stream(plugin_t * const plugin, Recorder *recorder, int16_t *samples, int sampleCount);
int plugin_call_start(plugin_t * const plugin, Call *call);
int plugin_call_end(plugin_t * const plugin, Call_Data_t call_info);
int plugin_calls_active(plugin_t * const plugin, std::vector<Call *> calls);
int plugin_setup_recorder(plugin_t * const plugin, Recorder *recorder);
int plugin_setup_system(plugin_t * const plugin, System * system);
int plugin_setup_systems(plugin_t * const plugin, std::vector<System *> systems);
int plugin_setup_sources(plugin_t * const plugin, std::vector<Source *> sources);
int plugin_setup_config(plugin_t * const plugin, std::vector<Source *> sources, std::vector<System *> systems);
int plugin_system_rates(plugin_t * const plugin, std::vector<System *> systems, float timeDiff);

#endif // PLUGIN_COMMON_H