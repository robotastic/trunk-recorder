#ifndef CALL_UPLOADER_H
#define CALL_UPLOADER_H

#include <vector>

class Call;

class System;

#include "../call.h"
#include "../config.h"
#include "../formatter.h"
#include "../systems/system.h"


struct call_data_t {
  long talkgroup;
  double freq;
  long start_time;
  long stop_time;
  bool encrypted;
  bool emergency;
  bool audio_archive;
  bool call_log;
  char filename[255];
  char status_filename[255];
  char converted[255];
  char file_path[255];
  std::string upload_server;
  std::string bcfy_api_key;
  std::string bcfy_calls_server;
  std::string api_key;
  std::string short_name;
  int bcfy_system_id;
  int tdma_slot;
  double length;
  bool phase2_tdma;
  long source_count;
  std::vector<Call_Source> source_list;
  long freq_count;
  Call_Freq freq_list[50];
  long error_list_count;
  Call_Error error_list[50];
};
Call_Data create_call_data(Call *call, System *sys, Config config);
void send_call(Call *call, System *sys, Config config);

#endif
