#ifndef GLOBAL_STRUCTS_H
#define GLOBAL_STRUCTS_H
#include <string>

struct Transmission {
  long source;
  long start_time;
  long stop_time;
  long sample_count;
  double freq;
  double length;
  char filename[255];
  char base_filename[255];
};



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
  bool enable_audio_streaming;
};



struct Call_Source {
  long source;
  long time;
  double position;
  bool emergency;
  std::string signal_system;
  std::string tag;
};

struct Call_Freq {
  double freq;
  long time;
  double position;
  double total_len;
  double error_count;
  double spike_count;
};

struct Call_Error {
  double freq;
  double sample_count;
  double error_count;
  double spike_count;
};

#endif
