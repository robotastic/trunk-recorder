#ifndef PLUGIN_API_H
#define PLUGIN_API_H

#include "../call_concluder/call_concluder.h"

typedef enum {
  PLUGIN_UNKNOWN,
  PLUGIN_INITIALIZED,
  PLUGIN_RUNNING,
  PLUGIN_FAILED,
  PLUGIN_STOPPED,
  PLUGIN_DISABLED
} plugin_state_t;

class Plugin_Api {
public:
  virtual int init(Config *config, std::vector<Source *> sources, std::vector<System *> systems) { return 0; };
  virtual int parse_config(boost::property_tree::ptree &cfg) { return 0; }; //const { BOOST_LOG_TRIVIAL(info) << "plugin_api created!";return 0; };
  virtual int start() { return 0; };
  virtual int stop() { return 0; };
  virtual int poll_one() { return 0; };
  virtual int signal(long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder) { return 0; };
  virtual int audio_stream(Call *call, Recorder *recorder, int16_t *samples, int sampleCount) { return 0; };
  virtual int call_start(Call *call) { return 0; };
  virtual int call_end(Call_Data_t call_info) { return 0; }; //= 0; //{ BOOST_LOG_TRIVIAL(info) << "plugin_api call_end"; return 0; };
  virtual int calls_active(std::vector<Call *> calls) { return 0; };
  virtual int setup_recorder(Recorder *recorder) { return 0; };
  virtual int setup_system(System *system) { return 0; };
  virtual int setup_systems(std::vector<System *> systems) { return 0; };
  virtual int setup_sources(std::vector<Source *> sources) { return 0; };
  virtual int setup_config(std::vector<Source *> sources, std::vector<System *> systems) { return 0; };
  virtual int system_rates(std::vector<System *> systems, float timeDiff) { return 0; };
  virtual int unit_registration(System *sys, long source_id) { return 0; };
  virtual int unit_deregistration(System *sys, long source_id) { return 0; };
  virtual int unit_acknowledge_response(System *sys, long source_id) { return 0; };
  virtual int unit_group_affiliation(System *sys, long source_id, long talkgroup_num) { return 0; };

  virtual ~Plugin_Api(){};
};

#endif