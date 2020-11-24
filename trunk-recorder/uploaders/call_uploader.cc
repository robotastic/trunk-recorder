#include "call_uploader.h"
#include "broadcastify_uploader.h"
#include "openmhz_uploader.h"
#include "uploader.h"
#include <boost/regex.hpp>

void *upload_call_thread(void *thread_arg) {
  call_data_t *call_info;

  pthread_detach(pthread_self());

  call_info = static_cast<call_data_t *>(thread_arg);

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
  }

  delete (call_info);
  return NULL;

  // pthread_exit(NULL);
}

void send_transmissions(Call *call, System *sys, Config config) {
  // struct call_data_t *call_info = (struct call_data_t *) malloc(sizeof(struct
  // call_data_t));
  for (std::size_t i = 0; i < call->transmission_list.size(); i++) {
    call_data_t *call_info = new call_data_t;
    pthread_t thread;

    // from: http://www.zedwood.com/article/cpp-boost-url-regex
    boost::regex ex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
    boost::cmatch what;

    strcpy(call_info->filename, call->transmission_list[i].filename);
    strcpy(call_info->converted, call->transmission_list[i].converted_filename);
    strcpy(call_info->status_filename, call->transmission_list[i].status_filename);
    strcpy(call_info->file_path, call->get_path());

    if (regex_match(config.upload_server.c_str(), what, ex)) {
      call_info->upload_server = config.upload_server;
    }
    if (regex_match(config.bcfy_calls_server.c_str(), what, ex)) {
      call_info->bcfy_calls_server = config.bcfy_calls_server;
    }

    // Exit if neither of our above URLs were valid
    if (config.upload_server.empty() && config.bcfy_calls_server.empty()) {
      BOOST_LOG_TRIVIAL(info) << "Unable to parse Server URL\n";
      return;
    }

    // std::cout << "Setting up thread\n";
    std::vector<Call_Source> source_list = call->get_source_list();
    Call_Freq *freq_list = call->get_freq_list();
    //Call_Error  *error_list  = call->get_error_list();
    call_info->talkgroup = call->get_talkgroup();
    call_info->freq = call->transmission_list[i].freq;
    call_info->encrypted = call->get_encrypted();
    call_info->emergency = call->get_emergency();
    call_info->tdma_slot = call->get_tdma_slot();
    call_info->phase2_tdma = call->get_phase2_tdma();
    call_info->error_list_count = call->get_error_list_count();
    call_info->source_count = 1;
    call_info->freq_count = 1;
    call_info->start_time = call->transmission_list[i].start_time;
    call_info->stop_time = call->transmission_list[i].stop_time;
    call_info->length = call->transmission_list[i].sample_count / 8000;
    call_info->api_key = sys->get_api_key();
    call_info->bcfy_api_key = sys->get_bcfy_api_key();
    call_info->bcfy_system_id = sys->get_bcfy_system_id();
    call_info->short_name = sys->get_short_name();
    call_info->audio_archive = sys->get_audio_archive();
    call_info->call_log = sys->get_call_log();
    Call_Source call_source = {call->transmission_list[i].source, call->transmission_list[i].start_time, 0, 0, "", ""};
    call_info->source_list.push_back(call_source);
    
    Call_Freq call_freq = {call->transmission_list[i].freq, call->transmission_list[i].start_time, 0};

      call_info->freq_list[0] = call_freq;
    
    BOOST_LOG_TRIVIAL(info) << "Trying to upload: " << call_info->converted << " Source: " << call->transmission_list[i].source;
    int rc = pthread_create(&thread, NULL, upload_call_thread, (void *)call_info);

    // pthread_detach(thread);

    if (rc) {
      printf("ERROR; return code from pthread_create() is %d", rc);
    }
  }
}

void send_call(Call *call, System *sys, Config config) {
  // struct call_data_t *call_info = (struct call_data_t *) malloc(sizeof(struct
  // call_data_t));
  call_data_t *call_info = new call_data_t;
  pthread_t thread;

  // from: http://www.zedwood.com/article/cpp-boost-url-regex
  boost::regex ex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
  boost::cmatch what;

  strcpy(call_info->filename, call->get_filename());
  strcpy(call_info->converted, call->get_converted_filename());
  strcpy(call_info->status_filename, call->get_status_filename());
  strcpy(call_info->file_path, call->get_path());

  if (regex_match(config.upload_server.c_str(), what, ex)) {
    call_info->upload_server = config.upload_server;
  }
  if (regex_match(config.bcfy_calls_server.c_str(), what, ex)) {
    call_info->bcfy_calls_server = config.bcfy_calls_server;
  }

  // Exit if neither of our above URLs were valid
  if (config.upload_server.empty() && config.bcfy_calls_server.empty()) {
    BOOST_LOG_TRIVIAL(info) << "Unable to parse Server URL\n";
    return;
  }

  // std::cout << "Setting up thread\n";
  std::vector<Call_Source> source_list = call->get_source_list();
  Call_Freq *freq_list = call->get_freq_list();
  //Call_Error  *error_list  = call->get_error_list();
  call_info->talkgroup = call->get_talkgroup();
  call_info->freq = call->get_freq();
  call_info->encrypted = call->get_encrypted();
  call_info->emergency = call->get_emergency();
  call_info->tdma_slot = call->get_tdma_slot();
  call_info->phase2_tdma = call->get_phase2_tdma();
  call_info->error_list_count = call->get_error_list_count();
  call_info->source_count = call->get_source_count();
  call_info->freq_count = call->get_freq_count();
  call_info->start_time = call->get_start_time();
  call_info->stop_time = call->get_stop_time();
  call_info->length = call->get_final_length();
  call_info->api_key = sys->get_api_key();
  call_info->bcfy_api_key = sys->get_bcfy_api_key();
  call_info->bcfy_system_id = sys->get_bcfy_system_id();
  call_info->short_name = sys->get_short_name();
  call_info->audio_archive = sys->get_audio_archive();
  call_info->call_log = sys->get_call_log();

  for (int i = 0; i < call_info->source_count; i++) {
    call_info->source_list.push_back(source_list[i]);
  }

  for (int i = 0; i < call_info->freq_count; i++) {
    call_info->freq_list[i] = freq_list[i];
  }

  int rc = pthread_create(&thread, NULL, upload_call_thread, (void *)call_info);

  // pthread_detach(thread);

  if (rc) {
    printf("ERROR; return code from pthread_create() is %d", rc);
  }
}
