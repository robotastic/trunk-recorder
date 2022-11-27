#ifndef GLOBAL_STRUCTS_H
#define GLOBAL_STRUCTS_H
#include <ctime>
#include <string>
#include <vector>

struct Transmission {
  long source;
  long start_time;
  long stop_time;
  long sample_count;
  long spike_count;
  long error_count;
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
  bool strict_csv_parsing;
  int debug_recorder_port;
  int call_timeout;
  bool log_file;
  int control_message_warn_rate;
  int control_retune_limit;
  bool broadcast_signals;
  bool enable_audio_streaming;
  bool record_uu_v_calls;
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
  long time;
  double position;
  double total_len;
  double error_count;
  double spike_count;
};

enum Call_Data_Status { INITIAL,
                        SUCCESS,
                        RETRY,
                        FAILED };
struct Call_Data_t {
  long talkgroup;
  std::vector<unsigned long> patched_talkgroups;
  std::string talkgroup_tag;
  std::string talkgroup_alpha_tag;
  std::string talkgroup_description;
  std::string talkgroup_group;
  long call_num;
  double freq;
  long start_time;
  long stop_time;
  long error_count;
  long spike_count;
  bool encrypted;
  bool emergency;
  bool audio_archive;
  bool transmission_archive;
  bool call_log;
  bool compress_wav;
  char filename[300];
  char status_filename[300];
  char converted[300];
  int min_transmissions_removed;

  std::string short_name;
  std::string upload_script;
  std::string audio_type;

  int tdma_slot;
  double length;
  bool phase2_tdma;

  std::vector<Call_Source> transmission_source_list;
  std::vector<Call_Error> transmission_error_list;
  std::vector<Transmission> transmission_list;

  Call_Data_Status status;
  time_t process_call_time;
  int retry_attempt;
};

#endif
