#include "call_concluder.h"
#include "../plugins/plugin-manager.h"

std::list<std::future<Call_Data_t>>  Call_Concluder::call_data_workers = {};
std::list<Call_Data_t> Call_Concluder::call_list = {};

int convert_media(char *filename, char* converted) {
  char shell_command[400];

  int nchars = snprintf(shell_command, 400, "sox %s -t wav - --norm=-3 | fdkaac --silent --ignorelength -b 8000 -o %s -", filename, converted); 

  if (nchars >= 400) {
    BOOST_LOG_TRIVIAL(error) << "Call uploader: Command longer than 400 characters";
    return -1;
  } 
    BOOST_LOG_TRIVIAL(trace) << "Converting: " << converted;
  BOOST_LOG_TRIVIAL(trace) << "Command: " << shell_command;

  int rc = system(shell_command);

  if (rc > 0) {
    BOOST_LOG_TRIVIAL(error) << "Failed to convert call recording, see above error. Make sure you have sox and fdkaac installed.";
    return -1;
  } else {
    BOOST_LOG_TRIVIAL(trace) << "Finished converting call";
  }
  return nchars;
  
}

Call_Data_t upload_call_worker(Call_Data_t call_info) {

  int result;
  BOOST_LOG_TRIVIAL(error) << "Converting media";
  // TR records files as .wav files. They need to be compressed before being upload to online services.
  result = convert_media(call_info.filename, call_info.converted);
  char shell_command[400];

  if (result < 0 ){
    call_info.status = failed;
    return call_info;
  }

  int error = 0;
/*
  if (call_info.bcfy_calls_server != "" && call_info.bcfy_api_key != "" && call_info.bcfy_system_id > 0) {
    BroadcastifyUploader *bcfy = new BroadcastifyUploader;
    error += bcfy->upload(call_info);
  }

  if (call_info.upload_server != "" && call_info.api_key != "") {
    OpenmhzUploader *openmhz = new OpenmhzUploader;
    error += openmhz->upload(call_info);
  }
*/
  plugman_call_end(call_info);
  BOOST_LOG_TRIVIAL(error) << "Status is: " << error;
  if (!error) {
    if (!call_info.audio_archive) {
      remove(call_info.filename);
      remove(call_info.converted);
    }
    if (!call_info.call_log) {
      remove(call_info.status_filename);
    }
    call_info.status = success;
  } else {
    call_info.status = retry;
  }


  return call_info;
}



Call_Data_t Call_Concluder::create_call_data(Call *call, System *sys, Config config) {
  Call_Data_t call_info;

  strcpy(call_info.filename, call->get_filename());
  strcpy(call_info.converted, call->get_converted_filename());
  strcpy(call_info.status_filename, call->get_status_filename());
  strcpy(call_info.file_path, call->get_path());

  // from: http://www.zedwood.com/article/cpp-boost-url-regex
  boost::regex ex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
  boost::cmatch what;

  if (regex_match(config.upload_server.c_str(), what, ex)) {
    call_info.upload_server = config.upload_server;
  }
  if (regex_match(config.bcfy_calls_server.c_str(), what, ex)) {
    call_info.bcfy_calls_server = config.bcfy_calls_server;
  }

  // Exit if neither of our above URLs were valid
  if (config.upload_server.empty() && config.bcfy_calls_server.empty()) {
    BOOST_LOG_TRIVIAL(info) << "Unable to parse Server URL\n";
    //return;
  }

  std::vector<Call_Source> source_list = call->get_source_list();
  Call_Freq *freq_list = call->get_freq_list();
  //Call_Error  *error_list  = call->get_error_list();
  call_info.talkgroup = call->get_talkgroup();
  call_info.freq = call->get_freq();
  call_info.encrypted = call->get_encrypted();
  call_info.emergency = call->get_emergency();
  call_info.tdma_slot = call->get_tdma_slot();
  call_info.phase2_tdma = call->get_phase2_tdma();
  call_info.error_list_count = call->get_error_list_count();
  call_info.source_count = call->get_source_count();
  call_info.freq_count = call->get_freq_count();
  call_info.start_time = call->get_start_time();
  call_info.stop_time = call->get_stop_time();
  call_info.length = call->get_final_length();
  call_info.api_key = sys->get_api_key();
  call_info.bcfy_api_key = sys->get_bcfy_api_key();
  call_info.bcfy_system_id = sys->get_bcfy_system_id();
  call_info.short_name = sys->get_short_name();
  call_info.audio_archive = sys->get_audio_archive();
  call_info.call_log = sys->get_call_log();

  for (int i = 0; i < call_info.source_count; i++) {
    call_info.source_list.push_back(source_list[i]);
  }

  for (int i = 0; i < call_info.freq_count; i++) {
    call_info.freq_list[i] = freq_list[i];
  }
  return call_info;
}

void Call_Concluder::conclude_call(Call *call, System *sys, Config config) {

  Call_Data_t call_info = create_call_data(call,sys,config);
  BOOST_LOG_TRIVIAL(info) << "Orig source list: " << call->get_source_list().size() << " Copied: " << call_info.source_list.size();

  BOOST_LOG_TRIVIAL(info) << "Worker list is: " << call_data_workers.size();
  call_data_workers.push_back( std::async( std::launch::async, upload_call_worker, call_info));
  BOOST_LOG_TRIVIAL(info) << "Now Worker list is: " << call_data_workers.size();
  for (std::list<std::future<Call_Data_t>>::iterator it = call_data_workers.begin(); it != call_data_workers.end();){

    if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
      Call_Data_t completed = it->get();
      BOOST_LOG_TRIVIAL(info) << completed.talkgroup << " / " << completed.freq << " Status: " << completed.status;
      it = call_data_workers.erase(it);
    } else {
      it++;
    }
  }
    BOOST_LOG_TRIVIAL(info) << "Finally, Worker list is: " << call_data_workers.size();
}