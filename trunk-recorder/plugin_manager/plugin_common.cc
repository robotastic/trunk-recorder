#include "plugin_common.h"
#include <assert.h>
#include <errno.h>
#include <string.h>

#include <boost/dll/import.hpp> // for import_alias
#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS   
#include <sys/types.h>


using namespace std;

typedef plugin_t * (*plugin_new_func_t)(void);

plugin_t *plugin_new(const char * const plugin_file, char const * const plugin_name) {
    assert(plugin_name != NULL);
    char *fname = NULL;
    assert(asprintf(&fname, "%s_plugin_new", plugin_name) > 0);
    plugin_new_func_t fptr = NULL;
    std::cout << "Load file: " << plugin_file << std::endl; 

    void *dlhandle = dlopen(plugin_file, RTLD_NOW);
    assert(dlhandle != NULL);
    fptr = (plugin_new_func_t)dlsym(dlhandle, fname);


    free(fname);
    if(fptr == NULL) {
        return NULL;
    }

    plugin_t *plugin = (*fptr)();
    assert(plugin->init != NULL);
    return plugin;
}

int plugin_init(plugin_t * const plugin, Config* config) {
    assert(plugin != NULL);
    plugin_state_t new_state = PLUGIN_FAILED;
    errno = 0;
    int ret = plugin->init(plugin, config);
    if(ret < 0) {
        ret = -1;
    } else {
        new_state = PLUGIN_INITIALIZED;
        ret = 0;
    }
    plugin.state = new_state;
    return ret;
}

int plugin_parse_config(plugin_t * const plugin, boost::property_tree::ptree::value_type &cfg) {
    assert(plugin != NULL);
    if(plugin->parse_config != NULL) {
        return plugin->parse_config(plugin, cfg);
    } else {
        return 0;
    }
}

int plugin_start(plugin_t * const plugin) {
    assert(plugin != NULL);
    assert(plugin->dev_data != NULL);
    int err = 0;
    errno = 0;
    if(plugin->state == PLUGIN_INITIALIZED && plugin->start != NULL) {
        err = plugin->start(plugin);
        if(err != 0) {
            plugin->state = PLUGIN_FAILED;
            return -1;
        }
    }
    plugin->state = PLUGIN_RUNNING;
    return 0;
}

int plugin_stop(plugin_t * const plugin) {
    assert(plugin != NULL);
    assert(plugin->dev_data != NULL);
    int err = 0;
    errno = 0;
    if(plugin->state == PLUGIN_RUNNING && plugin->stop != NULL) {
        err = plugin->stop(plugin);
        if(err != 0) {
            plugin->state = PLUGIN_FAILED;
            return -1;
        }
    }
    plugin->state = PLUGIN_STOPPED;
    return 0;
}

int plugin_poll_one(plugin_t * const plugin) {
    assert(plugin != NULL);

    int err = 0;
    errno = 0;

    if(plugin->state == PLUGIN_RUNNING && plugin->poll_one != NULL) {
        err = plugin->poll_one(plugin);
    }

    return err;
}

int plugin_signal(plugin_t * const plugin, long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder) {
    assert(plugin != NULL);

    int err = 0;
    errno = 0;

    if(plugin->state == PLUGIN_RUNNING && plugin->signal != NULL) {
        err = plugin->signal(plugin, unitId, signaling_type, sig_type, call, system, recorder);
    }

    return err;
}

int plugin_audio_stream(plugin_t * const plugin, Recorder* recorder, float *samples, int sampleCount) {
    assert(plugin != NULL);

    int err = 0;
    errno = 0;

    if(plugin->state == PLUGIN_RUNNING && plugin->audio_stream != NULL) {
        err = plugin->audio_stream(plugin, recorder, samples, sampleCount);
    }
    return err;
}

int plugin_call_start(plugin_t * const plugin, Call *call) {
    assert(plugin != NULL);

    int err = 0;
    errno = 0;

    if(plugin->state == PLUGIN_RUNNING && plugin->call_start != NULL) {
        err = plugin->call_start(plugin, call);
    }

    return err;
}

int plugin_call_end(plugin_t * const plugin, Call_Data_t call_info) {
    assert(plugin != NULL);

    int err = 0;
    errno = 0;

    if(plugin->state == PLUGIN_RUNNING && plugin->call_end != NULL) {
        err = plugin->call_end(plugin, call_info);
    }

    return err;
}

int plugin_calls_active(plugin_t * const plugin, std::vector<Call *> calls) {
    assert(plugin != NULL);

    int err = 0;
    errno = 0;

    if(plugin->state == PLUGIN_RUNNING && plugin->calls_active != NULL) {
        err = plugin->calls_active(plugin, calls);
    }

    return err;
}

int plugin_setup_recorder(plugin_t * const plugin, Recorder *recorder) {
    assert(plugin != NULL);

    int err = 0;
    errno = 0;

    if(plugin->state == PLUGIN_RUNNING && plugin->setup_recorder != NULL) {
        err = plugin->setup_recorder(plugin, recorder);
    }

    return err;
}

int plugin_setup_system(plugin_t * const plugin, System * system) {
    assert(plugin != NULL);

    int err = 0;
    errno = 0;

    if(plugin->state == PLUGIN_RUNNING && plugin->setup_system != NULL) {
        err = plugin->setup_system(plugin, system);
    }

    return err;
}

int plugin_setup_systems(plugin_t * const plugin, std::vector<System *> systems) {
    assert(plugin != NULL);

    int err = 0;
    errno = 0;

    if(plugin->state == PLUGIN_RUNNING && plugin->setup_systems != NULL) {
        err = plugin->setup_systems(plugin, systems);
    }

    return err;
}

int plugin_setup_sources(plugin_t * const plugin, std::vector<Source *> sources) {
    assert(plugin != NULL);

    int err = 0;
    errno = 0;

    if(plugin->state == PLUGIN_RUNNING && plugin->setup_sources != NULL) {
        err = plugin->setup_sources(plugin, sources);
    }

    return err;
}

int plugin_setup_config(plugin_t * const plugin, std::vector<Source *> sources, std::vector<System *> systems) {
    assert(plugin != NULL);

    int err = 0;
    errno = 0;

    if(plugin->state == PLUGIN_RUNNING && plugin->setup_config != NULL) {
        err = plugin->setup_config(plugin, sources, systems);
    }

    return err;
}

int plugin_system_rates(plugin_t * const plugin, std::vector<System *> systems, float timeDiff) {
    assert(plugin != NULL);

    int err = 0;
    errno = 0;

    if(plugin->state == PLUGIN_RUNNING && plugin->system_rates != NULL) {
        err = plugin->system_rates(plugin, systems, timeDiff);
    }

    return err;
}