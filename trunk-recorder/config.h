#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config {
  std::string upload_script;
  std::string upload_server;
  std::string bcfy_calls_server;
  std::string status_server;
  std::string instance_key;
  std::string instance_id;
  std::string capture_dir;
  std::string debug_recorder_address;
  std::string log_dir;
  bool debug_recorder;
  int debug_recorder_port;
  int call_timeout;
  bool log_file;
  int control_message_warn_rate;
  int control_retune_limit;
  bool broadcast_signals;
};


#endif
