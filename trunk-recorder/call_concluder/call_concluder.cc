#include "call_concluder.h"
#include "../plugin_manager/plugin_manager.h"
#include <boost/filesystem.hpp>


std::list<std::future<Call_Data_t>> Call_Concluder::call_data_workers = {};
std::list<Call_Data_t> Call_Concluder::retry_call_list = {};

int combine_wav(std::string files, char *target_filename) {
  char shell_command[4000];

  int nchars = snprintf(shell_command, 4000, "sox %s %s --norm=-.01", files.c_str(), target_filename);

  if (nchars >= 4000) {
    BOOST_LOG_TRIVIAL(error) << "Call uploader: SOX Combine WAV Command longer than 4000 characters";
    return -1;
  }
  int rc = system(shell_command);



  if (rc > 0) {
    BOOST_LOG_TRIVIAL(info) << "Combining: " << files.c_str() << " into: " << target_filename;
    BOOST_LOG_TRIVIAL(info) << shell_command;
    BOOST_LOG_TRIVIAL(error) << "Failed to combine recordings, see above error. Make sure you have sox and fdkaac installed.";
    return -1;
  } 

  return nchars;
}
int convert_media(char *filename, char *converted) {
  char shell_command[400];

  int nchars = snprintf(shell_command, 400, "sox %s -t wav - | fdkaac --silent  -p 2 --ignorelength -b 8000 -o %s -", filename, converted);

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

int create_call_json(Call_Data_t call_info) {
  // Create the JSON status file
  std::ofstream json_file(call_info.status_filename);

  if (json_file.is_open()) {
    json_file << "{\n";
    json_file << "\"freq\": " << std::fixed << std::setprecision(0) << call_info.freq << ",\n";
    json_file << "\"start_time\": " << call_info.start_time << ",\n";
    json_file << "\"stop_time\": " << call_info.stop_time << ",\n";
    json_file << "\"emergency\": " << call_info.emergency << ",\n";
    json_file << "\"encrypted\": " << call_info.encrypted << ",\n";
    json_file << "\"call_length\": " << call_info.length << ",\n";
    //json_file << "\"source\": \"" << this->get_recorder()->get_source()->get_device() << "\",\n";
    json_file << "\"talkgroup\": " << call_info.talkgroup << ",\n";
    json_file << "\"talkgroup_tag\": \"" << call_info.talkgroup_alpha_tag << "\",\n";
    json_file << "\"talkgroup_description\": \"" << call_info.talkgroup_description << "\",\n";
    json_file << "\"talkgroup_group_tag\": \"" << call_info.talkgroup_tag << "\",\n";
    json_file << "\"talkgroup_group\": \"" << call_info.talkgroup_group << "\",\n";
    json_file << "\"audio_type\": \"" << call_info.audio_type << "\",\n";
    json_file << "\"short_name\": \"" << call_info.short_name << "\",\n";
    if (call_info.patched_talkgroups.size()>1){
      json_file << "\"patched_talkgroups\": [";
      bool first = true;
      BOOST_FOREACH (auto& TGID, call_info.patched_talkgroups) {
        if (!first) { json_file << ","; }
        first = false;
        json_file << (int)TGID;
      }
      json_file << "],\n";
    }

    json_file << "\"freqList\": [";
    json_file << "{ \"freq\": " << std::fixed << std::setprecision(0) << call_info.freq << ", \"time\": " << call_info.start_time << ", \"pos\": 0.0, \"len\": " << call_info.length << ", \"error_count\": 0.0, \"spike_count\": 0.0}";
    json_file << "],\n";
    json_file << "\"srcList\": [ ";

    for (std::size_t i = 0; i < call_info.transmission_source_list.size(); i++) {
      if (i != 0) {
        json_file << ", ";
      }
      json_file << "{\"src\": " << std::fixed << call_info.transmission_source_list[i].source << ", \"time\": " << call_info.transmission_source_list[i].time << ", \"pos\": " << call_info.transmission_source_list[i].position << ", \"emergency\": " << call_info.transmission_source_list[i].emergency << ", \"signal_system\": \"" << call_info.transmission_source_list[i].signal_system << "\", \"tag\": \"" << call_info.transmission_source_list[i].tag << "\"}"; 
    }
    json_file << " ]\n";
    json_file << "}\n";
    json_file.close();
    return 0;
  } else {
    BOOST_LOG_TRIVIAL(error) << "[" << call_info.short_name << "]\t\033[0;34m" << call_info.call_num << "C\033[0m \t Unable to create JSON file: " << call_info.status_filename;
    return 1;
  }
}

bool checkIfFile(std::string filePath)
{
    try {
        // Create a Path object from given path string
        boost::filesystem::path pathObj(filePath);
        // Check if path exists and is of a regular file
        if (boost::filesystem::exists(pathObj) && boost::filesystem::is_regular_file(pathObj))
            return true;
    }
    catch (boost::filesystem::filesystem_error & e)
    {
        BOOST_LOG_TRIVIAL(error) << e.what() << std::endl;
    }
    return false;
}

void remove_call_files(Call_Data_t call_info) {

  if (!call_info.audio_archive) {
    if(checkIfFile(call_info.filename)) {
      remove(call_info.filename);
    }
    if(checkIfFile(call_info.converted)) {
    remove(call_info.converted);
    }
    for (std::vector<Transmission>::iterator it = call_info.transmission_list.begin(); it != call_info.transmission_list.end(); ++it) {
      Transmission t = *it;
      if(checkIfFile(t.filename)) {
      remove(t.filename);
      }
    }
  } else if (!call_info.transmission_archive) {
    for (std::vector<Transmission>::iterator it = call_info.transmission_list.begin(); it != call_info.transmission_list.end(); ++it) {
      Transmission t = *it;
      if(checkIfFile(t.filename)) {
      remove(t.filename);
      }
    }    
  }
  if (!call_info.call_log) {
    if(checkIfFile(call_info.status_filename)) {
    remove(call_info.status_filename);
    }
  }
}

Call_Data_t upload_call_worker(Call_Data_t call_info) {
  int result;

  if (call_info.status == INITIAL) {
    std::stringstream shell_command;
    std::string shell_command_string;
    std::string files;

    int filename_length;

    char formattedTalkgroup[62];
    snprintf(formattedTalkgroup, 61, "%c[%dm%10ld%c[0m", 0x1B, 35, call_info.talkgroup, 0x1B);
    std::string talkgroup_display = boost::lexical_cast<std::string>(formattedTalkgroup);

    // loop through the transmission list, pull in things to fill in totals for call_info
    // Using a for loop with iterator
    for (std::vector<Transmission>::iterator it = call_info.transmission_list.begin(); it != call_info.transmission_list.end(); ++it) {
      Transmission t = *it;

      files.append(t.filename);
      files.append(" ");
    }

    combine_wav(files, call_info.filename);

    result = create_call_json(call_info);

    if (result < 0) {
      call_info.status = FAILED;
      return call_info;
    }

    if (call_info.compress_wav) {
    // TR records files as .wav files. They need to be compressed before being upload to online services.
      result = convert_media(call_info.filename, call_info.converted);
      
      if (result < 0) {
        call_info.status = FAILED;
        return call_info;
      }
    } 


    // Handle the Upload Script, if set
    if (call_info.upload_script.length() != 0) {
      shell_command << "./" << call_info.upload_script << " " << call_info.filename << " " << call_info.status_filename << " " << call_info.converted;
      shell_command_string = shell_command.str();

      BOOST_LOG_TRIVIAL(info) << "[" << call_info.short_name << "]\t\033[0;34m" << call_info.call_num << "C\033[0m \t Running upload script: " << shell_command_string;
      //signal(SIGCHLD, SIG_IGN);

      result = system(shell_command_string.c_str());
    }
  }

  int error = 0;

  error = plugman_call_end(call_info);

  if (!error) {
    remove_call_files(call_info);
    call_info.status = SUCCESS;
  } else {
    call_info.status = RETRY;
  }

  return call_info;
}

Call_Data_t Call_Concluder::create_call_data(Call *call, System *sys, Config config) {
  Call_Data_t call_info;
  double total_length = 0;
  
  call_info.status = INITIAL;
  call_info.process_call_time = time(0);
  call_info.retry_attempt = 0;

  

  call_info.freq = call->get_freq();
  call_info.encrypted = call->get_encrypted();
  call_info.emergency = call->get_emergency();
  call_info.tdma_slot = call->get_tdma_slot();
  call_info.phase2_tdma = call->get_phase2_tdma();
  call_info.transmission_list = call->transmission_list;
  call_info.short_name = sys->get_short_name();
  call_info.upload_script = sys->get_upload_script();
  call_info.audio_archive = sys->get_audio_archive();
  call_info.transmission_archive = sys->get_transmission_archive();
  call_info.call_log = sys->get_call_log();
  call_info.call_num = call->get_call_num();
  call_info.compress_wav = sys->get_compress_wav();
  
  call_info.talkgroup = call->get_talkgroup();
  call_info.patched_talkgroups = sys->get_talkgroup_patch(call_info.talkgroup);
  
  Talkgroup *tg = sys->find_talkgroup(call->get_talkgroup());
  if (tg!=NULL) {
    call_info.talkgroup_tag = tg->tag;
    call_info.talkgroup_alpha_tag = tg->alpha_tag;
    call_info.talkgroup_description = tg->description;
    call_info.talkgroup_group = tg->group;
  } else {
    call_info.talkgroup_tag = "";
    call_info.talkgroup_alpha_tag = "";
    call_info.talkgroup_description = "";
    call_info.talkgroup_group = "";
  }


  if (call->get_is_analog()) {
    call_info.audio_type = "analog";
  } else if (call->get_phase2_tdma()) {
    call_info.audio_type = "digital tdma";
  } else {
    call_info.audio_type = "digital";
  }

    // loop through the transmission list, pull in things to fill in totals for call_info
    // Using a for loop with iterator
    for (std::vector<Transmission>::iterator it = call_info.transmission_list.begin(); it != call_info.transmission_list.end(); ++it) {
      Transmission t = *it;
      char formattedTalkgroup[62];
      snprintf(formattedTalkgroup, 61, "%c[%dm%10ld%c[0m", 0x1B, 35, call_info.talkgroup, 0x1B);
      std::string talkgroup_display = boost::lexical_cast<std::string>(formattedTalkgroup);
      BOOST_LOG_TRIVIAL(info) << "[" << call_info.short_name << "]\t\033[0;34m" << call_info.call_num << "C\033[0m\tTG: " << talkgroup_display << "\tFreq: " << format_freq(call_info.freq) << "\t- Transmission src: " << t.source << " pos: " << total_length << " length: " << t.length;
      
      if (it == call_info.transmission_list.begin()) {
        call_info.start_time = t.start_time;
        snprintf(call_info.filename,300,"%s-call_%lu.wav",t.base_filename,call_info.call_num);
        snprintf(call_info.status_filename,300,"%s-call_%lu.json",t.base_filename,call_info.call_num);
        snprintf(call_info.converted,300,"%s-call_%lu.m4a",t.base_filename,call_info.call_num);
      } 

      if (std::next(it) == call_info.transmission_list.end()) {
        call_info.stop_time = t.stop_time;
      }

      UnitTag *unit_tag = sys->find_unit_tag(t.source);
      std::string tag = (unit_tag == NULL || unit_tag->tag.empty() ? "" : unit_tag->tag);
      Call_Source call_source = {t.source, t.start_time, total_length, false, "", tag};

      call_info.transmission_source_list.push_back(call_source);

      total_length = total_length + t.length;
    }

    call_info.length = total_length;


  return call_info;
}

void Call_Concluder::conclude_call(Call *call, System *sys, Config config) {
  char formattedTalkgroup[62];

  Call_Data_t call_info = create_call_data(call, sys, config);

  snprintf(formattedTalkgroup, 61, "%c[%dm%10ld%c[0m", 0x1B, 35, call_info.talkgroup, 0x1B);
  std::string talkgroup_display = boost::lexical_cast<std::string>(formattedTalkgroup);


  if (call_info.transmission_list.size() == 0) {
    BOOST_LOG_TRIVIAL(error) << "[" << call_info.short_name << "]\t\033[0;34m" << call_info.call_num << "C\033[0m\tTG: " << talkgroup_display  << "\t Freq: " << call_info.freq << "\tNo Transmission were recorded!";
    return;
  }

  if (call_info.length <= sys->get_min_duration()) {
    BOOST_LOG_TRIVIAL(error) << "[" << call_info.short_name << "]\t\033[0;34m" << call_info.call_num << "C\033[0m\tTG: " << talkgroup_display  << "\t Freq: " << call_info.freq << "\t Call length: " << call_info.length << " is less than min duration: " << sys->get_min_duration();
    remove_call_files(call_info);
    return;
  }

  
  
  call_data_workers.push_back(std::async(std::launch::async, upload_call_worker, call_info));
  
  
}

void Call_Concluder::manage_call_data_workers() {
  for (std::list<std::future<Call_Data_t>>::iterator it = call_data_workers.begin(); it != call_data_workers.end();) {

    if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
      Call_Data_t call_info = it->get();
      if (call_info.status == RETRY) {
        call_info.retry_attempt++;
        char formattedTalkgroup[62];
        snprintf(formattedTalkgroup, 61, "%c[%dm%10ld%c[0m", 0x1B, 35, call_info.talkgroup, 0x1B);
        std::string talkgroup_display = boost::lexical_cast<std::string>(formattedTalkgroup);
        time_t start_time = call_info.start_time;
        if (call_info.retry_attempt > Call_Concluder::MAX_RETRY) {
          remove_call_files(call_info);
          BOOST_LOG_TRIVIAL(error) << "[" << call_info.short_name << "]\t\033[0;34m" << call_info.call_num << "C\033[0m Failed to conclude call - TG: " << talkgroup_display << "\t" << std::put_time(std::localtime(&start_time), "%c %Z");

        } else {
          long jitter = rand() % 10;
          long backoff = (2 ^ call_info.retry_attempt * 60) + jitter;
          call_info.process_call_time = time(0) + backoff;
          retry_call_list.push_back(call_info);
          BOOST_LOG_TRIVIAL(error) << "[" << call_info.short_name << "]\t\033[0;34m" << call_info.call_num << "C\033[0m \tTG: " << talkgroup_display << "\t" << std::put_time(std::localtime(&start_time), "%c %Z") << " retry attempt " << call_info.retry_attempt << " in " << backoff << "s\t retry queue: " << retry_call_list.size() << " calls";
        }
      }
      it = call_data_workers.erase(it);
    } else {
      it++;
    }
  }
  for (std::list<Call_Data_t>::iterator it = retry_call_list.begin(); it != retry_call_list.end();) {
    Call_Data_t call_info = *it;
    if (call_info.process_call_time <= time(0)) {
      call_data_workers.push_back(std::async(std::launch::async, upload_call_worker, call_info));
      it = retry_call_list.erase(it);
    } else {
      it++;
    }
  }
}