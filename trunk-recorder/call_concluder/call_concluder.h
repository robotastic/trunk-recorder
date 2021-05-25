#ifndef CALL_CONCLUDER_H
#define CALL_CONCLUDER_H
#include <queue>
#include <list>
#include <vector>
#include <thread>
#include <future>
#include <boost/regex.hpp>

#include "../call.h"
#include "../config.h"
#include "../formatter.h"
#include "../systems/system.h"
/*
class Uploader;
#include "../uploaders/uploader.h"
#include "../uploaders/call_uploader.h"
#include "../uploaders/broadcastify_uploader.h"
#include "../uploaders/openmhz_uploader.h"*/


struct Call_Data {
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
Call_Data upload_call_worker(Call_Data call_info);
void conclude_call(Call *call, System *sys, Config config);


#endif