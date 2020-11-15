#include "plugin-manager.h"
#include "plugin-common.h"
#include <stdlib.h>
#include <vector>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/log/trivial.hpp>

std::vector<plugin_t *> plugins;

void initialize_plugins(boost::property_tree::ptree &cfg) {
    BOOST_FOREACH (boost::property_tree::ptree::value_type &node, cfg.get_child("plugins")) {
        std::string plugin_lib = node.second.get<std::string>("library", "");
        std::string plugin_name = node.second.get<std::string>("name", "");

        plugin_t *plugin = plugin_new(plugin_lib == "" ? NULL : plugin_lib.c_str(), plugin_name.c_str());
        if(plugin != NULL && plugin_parse_config(plugin, node) == 0) {
            plugins.push_back(plugin);
        }
    }

    for (std::vector<plugin_t *>::iterator it = plugins.begin(); it != plugins.end(); it++) {
      plugin_t *plugin = *it;
      plugin_init(plugin);
    }
}

void start_plugins(std::vector<Source *> sources, std::vector<System *> systems) {
    for (std::vector<plugin_t *>::iterator it = plugins.begin(); it != plugins.end(); it++) {
      plugin_t *plugin = *it;
      plugin_start(plugin);
      plugin_setup_sources(plugin, sources);
      plugin_setup_systems(plugin, systems);
    }
}

void stop_plugins() {
    for (std::vector<plugin_t *>::iterator it = plugins.begin(); it != plugins.end(); it++) {
      plugin_t *plugin = *it;
      plugin_stop(plugin);
    }
}

void plugman_poll_one() {
    for (std::vector<plugin_t *>::iterator it = plugins.begin(); it != plugins.end(); it++) {
      plugin_t *plugin = *it;
      plugin_poll_one(plugin);
    }
}

void plugman_audio_callback(Recorder *recorder, float *samples, int sampleCount) {
    for (std::vector<plugin_t *>::iterator it = plugins.begin(); it != plugins.end(); it++) {
      plugin_t *plugin = *it;
      plugin_audio_stream(plugin, recorder, samples, sampleCount);
    }
}

void plugman_signal(long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder) {
    for (std::vector<plugin_t *>::iterator it = plugins.begin(); it != plugins.end(); it++) {
      plugin_t *plugin = *it;
      plugin_signal(plugin, unitId, signaling_type, sig_type, call, system, recorder);
    }
}
void plugman_call_start(Call *call) {
    for (std::vector<plugin_t *>::iterator it = plugins.begin(); it != plugins.end(); it++) {
      plugin_t *plugin = *it;
      plugin_call_start(plugin, call);
    }
}
void plugman_call_end(Call *call) {
    for (std::vector<plugin_t *>::iterator it = plugins.begin(); it != plugins.end(); it++) {
      plugin_t *plugin = *it;
      plugin_call_end(plugin, call);
    }
}

void plugman_setup_recorder(Recorder *recorder) {
    for (std::vector<plugin_t *>::iterator it = plugins.begin(); it != plugins.end(); it++) {
      plugin_t *plugin = *it;
      plugin_setup_recorder(plugin, recorder);
    }
}

void plugman_setup_system(System * system) {
    for (std::vector<plugin_t *>::iterator it = plugins.begin(); it != plugins.end(); it++) {
      plugin_t *plugin = *it;
      plugin_setup_system(plugin, system);
    }
}

void plugman_setup_systems(std::vector<System *> systems) {
    for (std::vector<plugin_t *>::iterator it = plugins.begin(); it != plugins.end(); it++) {
      plugin_t *plugin = *it;
      plugin_setup_systems(plugin, systems);
    }
}

void plugman_setup_sources(std::vector<Source *> sources) {
    for (std::vector<plugin_t *>::iterator it = plugins.begin(); it != plugins.end(); it++) {
      plugin_t *plugin = *it;
      plugin_setup_sources(plugin, sources);
    }
}