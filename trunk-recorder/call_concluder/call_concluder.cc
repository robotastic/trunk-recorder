#include "call_concluder.h"


static std::list<Call_Data> call_list;
static std::list<std::future<Call_Data>> call_data_workers;


Call_Data upload_call_worker(Call_Data call_info) {
/*
  char shell_command[400];

  //int nchars = snprintf(shell_command, 400, "nice -n -10 ffmpeg -y -i %s  -c:a libfdk_aac -b:a 32k -filter:a \"volume=15db\" -filter:a loudnorm -cutoff 18000 -hide_banner -loglevel panic %s ", call_info->filename, call_info->converted);
  //int nchars = snprintf(shell_command, 400, "ffmpeg -y -i %s  -c:a libfdk_aac -b:a 32k -filter:a \"volume=15db\" -filter:a loudnorm  -hide_banner -loglevel panic %s ", call_info->filename, call_info->converted);
  //int nchars = snprintf(shell_command, 400, "cd %s && fdkaac -S -b16 --raw-channels 1 --raw-rate 8000 %s", call_info->file_path, call_info->filename);
  //hints from here https://github.com/nu774/fdkaac/issues/5 on how to pipe between the 2
  int nchars = snprintf(shell_command, 400, "sox %s -t wav - --norm=-3 | fdkaac --silent --ignorelength -b 8000 -o %s -", call_info->filename, call_info->converted);

  if (nchars >= 400) {
    BOOST_LOG_TRIVIAL(error) << "Call uploader: Command longer than 400 characters";
    delete (call_info);
    return NULL;
  }

  BOOST_LOG_TRIVIAL(trace) << "Converting: " << call_info->converted;
  BOOST_LOG_TRIVIAL(trace) << "Command: " << shell_command;

  int rc = system(shell_command);

  if (rc > 0) {
    BOOST_LOG_TRIVIAL(error) << "Failed to convert call recording, see above error. Make sure you have sox and fdkaac installed.";
    delete (call_info);
    return NULL;
  } else {
    BOOST_LOG_TRIVIAL(trace) << "Finished converting call";
  }

  int error = 0;

  if (call_info->bcfy_calls_server != "" && call_info->bcfy_api_key != "" && call_info->bcfy_system_id > 0) {
    BroadcastifyUploader *bcfy = new BroadcastifyUploader;
    error += bcfy->upload(call_info);
  }

  if (call_info->upload_server != "" && call_info->api_key != "") {
    OpenmhzUploader *openmhz = new OpenmhzUploader;
    error += openmhz->upload(call_info);
  }

  if (!error) {
    if (!call_info->audio_archive) {
      remove(call_info->filename);
      remove(call_info->converted);
    }
    if (!call_info->call_log) {
      remove(call_info->status_filename);
    }
  } else {
    return call_info;
  }
*/

  return call_info;

  // pthread_exit(NULL);
}



Call_Data create_call_data(Call *call, System *sys, Config config) {
  Call_Data call_info;

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

  // std::cout << "Setting up thread\n";
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

void conclude_call(Call *call, System *sys, Config config) {

  Call_Data call_info = create_call_data(call,sys,config);
  BOOST_LOG_TRIVIAL(info) << "Orig source list: " << call->get_source_list().size() << " Copied: " << call_info.source_list.size();
  call_list.push_back(call_info);
    BOOST_LOG_TRIVIAL(info) << "Worker list is: " << call_data_workers.size();
  call_data_workers.push_back( std::async( std::launch::async, upload_call_worker, call_info));
  BOOST_LOG_TRIVIAL(info) << "Now Worker list is: " << call_data_workers.size();
  for (std::list<std::future<Call_Data>>::iterator it = call_data_workers.begin(); it != call_data_workers.end();){

    if (it->wait_for(std::chrono::seconds(1)) == std::future_status::ready) {
      Call_Data completed = it->get();
      BOOST_LOG_TRIVIAL(info) << completed.talkgroup << " is all done " << completed.freq;
      it = call_data_workers.erase(it);
    } else {
      it++;
    }
  }
    BOOST_LOG_TRIVIAL(info) << "Finally, Worker list is: " << call_data_workers.size();
}