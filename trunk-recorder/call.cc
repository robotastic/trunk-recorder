#include "call.h"

void Call::create_filename() {
  tm *ltm = localtime(&start_time);


  std::stringstream path_stream;

  path_stream << this->config.capture_dir << "/" << sys->get_short_name() << "/" << 1900 + ltm->tm_year << "/" <<  1 + ltm->tm_mon << "/" << ltm->tm_mday;

  boost::filesystem::create_directories(path_stream.str());
  int nchars;
  nchars = snprintf(filename,   255,        "%s/%ld-%ld_%g.wav",  path_stream.str().c_str(), talkgroup, start_time, curr_freq);

  if (nchars >= 255) {
    BOOST_LOG_TRIVIAL(error) << "Call: Path longer than 160 charecters";
  }
  nchars = snprintf(status_filename,  255,  "%s/%ld-%ld_%g.json", path_stream.str().c_str(), talkgroup, start_time, curr_freq);

  if (nchars >= 255) {
    BOOST_LOG_TRIVIAL(error) << "Call: Path longer than 160 charecters";
  }
  nchars = snprintf(converted_filename, 255, "%s/%ld-%ld.m4a",     path_stream.str().c_str(), talkgroup, start_time);

  if (nchars >= 255) {
    BOOST_LOG_TRIVIAL(error) << "Call: Path longer than 255 charecters";
  }

  // sprintf(filename, "%s/%ld-%ld.wav",
  // path_stream.str().c_str(),talkgroup,start_time);
  // sprintf(status_filename, "%s/%ld-%ld.json",
  // path_stream.str().c_str(),talkgroup,start_time);
}

Call::Call(long t, double f, System *s, Config c) {
  config           = c;
  idle_count       = 0;
  freq_count       = 0;
  error_list_count = 0;
  curr_freq        = 0;
  src_count        = 0;
  curr_src_id      = 0;
  talkgroup       = t;
  sys             = s;
  start_time      = time(NULL);
  stop_time       = time(NULL);
  last_update     = time(NULL);
  state           = monitoring;
  debug_recording = false;
  recorder        = NULL;
  phase2_tdma     = false;
  tdma_slot       = 0;
  encrypted       = false;
  emergency       = false;
  conventional    = false;
  set_freq(f);
  this->create_filename();
}

Call::Call(TrunkMessage message, System *s, Config c) {
  config           = c;
  idle_count       = 0;
  freq_count       = 0;
  error_list_count = 0;
  src_count        = 0;
  curr_src_id      = 0;
  curr_freq        = 0;
  talkgroup  = message.talkgroup;
  sys        = s;
  start_time = time(NULL);
  stop_time       = time(NULL);
  last_update     = time(NULL);
  state           = monitoring;
  debug_recording = false;
  recorder        = NULL;
  phase2_tdma     = message.phase2_tdma;
  tdma_slot       = message.tdma_slot;
  encrypted       = message.encrypted;
  emergency       = message.emergency;
  conventional    = false;
  set_freq(message.freq);
  add_source(message.source);
  this->create_filename();
}

Call::~Call() {
  //  BOOST_LOG_TRIVIAL(info) << " This call is over!!";
}

void Call::restart_call() {
  if (conventional) {
    idle_count       = 0;
    freq_count       = 0;
    src_count        = 0;
    error_list_count = 0;
    curr_src_id      = 0;
    start_time       = time(NULL);
    stop_time        = time(NULL);
    last_update      = time(NULL);
    state            = recording;
    debug_recording  = false;
    phase2_tdma      = false;
    tdma_slot        = 0;
    encrypted        = false;
    emergency        = false;

    this->create_filename();
    recorder->start(this, talkgroup + 100);
  }
}

void Call::end_call() {
  std::stringstream shell_command;
  stop_time = time(NULL);

  if (state == recording) {
    if (!recorder) {
      BOOST_LOG_TRIVIAL(error) << "Call::end_call() State is recording, but no recorder assigned!";
    }
    BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\tTG: " << get_talkgroup() << "\tFreq: " << get_freq() << "\tEnding Recorded Call - Last Update: " << this->since_last_update() << "s\tCall Elapsed: " << this->elapsed();;



    if (freq_count > 0) {
      Rx_Status rx_status = recorder->get_rx_status();
      freq_list[freq_count - 1].total_len   = rx_status.total_len;
      freq_list[freq_count - 1].spike_count = rx_status.spike_count;
      freq_list[freq_count - 1].error_count = rx_status.error_count;
    }

    if (sys->get_call_log()) {
      std::ofstream myfile(status_filename);


      if (myfile.is_open())
      {
        myfile << "{\n";
        myfile << "\"freq\": " << this->curr_freq << ",\n";
        myfile << "\"start_time\": " << this->start_time << ",\n";
        myfile << "\"stop_time\": " << this->stop_time << ",\n";
        myfile << "\"emergency\": " << this->emergency << ",\n";
        //myfile << "\"source\": \"" << this->get_recorder()->get_source()->get_device() << "\",\n";
        myfile << "\"talkgroup\": " << this->talkgroup << ",\n";
        myfile << "\"srcList\": [ ";

        for (int i = 0; i < src_count; i++) {
          if (i != 0) {
            myfile << ", ";
          }
          myfile << "{\"src\": " << std::fixed << src_list[i].source << ", \"time\": " << src_list[i].time << ", \"pos\": " << src_list[i].position << "}";
        }
        myfile << " ],\n";
        myfile << "\"freqList\": [ ";

        for (int i = 0; i < freq_count; i++) {
          if (i != 0) {
            myfile << ", ";
          }
          myfile << "{ \"freq\": " << std::fixed <<  freq_list[i].freq << ", \"time\": " << freq_list[i].time << ", \"pos\": " << freq_list[i].position << ", \"len\": " << freq_list[i].total_len << ", \"error_count\": " << freq_list[i].error_count << ", \"spike_count\": " << freq_list[i].spike_count << "}";
        }
        myfile << " ]\n";
        myfile << "}\n";
        myfile.close();
      }
    }

    if (sys->get_upload_script().length() != 0) {
      shell_command << "./" << sys->get_upload_script() << " " << this->get_filename() << " &";
    }
    this->get_recorder()->stop();

    if (this->config.upload_server != "") {
      send_call(this, sys, config);
    } else {}

    if (sys->get_upload_script().length() != 0) {
      BOOST_LOG_TRIVIAL(info) << "Running upload script: " << shell_command.str();
      int rc = system(shell_command.str().c_str());
    }
  }

  if (this->get_debug_recording() == true) {
    this->get_debug_recorder()->stop();
  }
}

void Call::set_debug_recorder(Recorder *r) {
  debug_recorder = r;
}

Recorder * Call::get_debug_recorder() {
  return debug_recorder;
}

void Call::set_recorder(Recorder *r) {
  recorder = r;
}

Recorder * Call::get_recorder() {
  return recorder;
}

double Call::get_freq() {
  return curr_freq;
}

double Call::get_current_length() {
  if ((state == recording) && recorder)  {
    if (!recorder) {
      BOOST_LOG_TRIVIAL(error) << "Call::get_current_length() State is recording, but no recorder assigned!";
    }
    return get_recorder()->get_current_length(); // This could SegFault
  } else {
    return time(NULL) - start_time;
  }
}

void Call::set_error(Rx_Status rx_status) {
  Call_Error call_error = { curr_freq, rx_status.total_len, rx_status.error_count, rx_status.spike_count };

  if (error_list_count < 49) {
    error_list[error_list_count] = call_error;
    error_list_count++;
  } else {
    BOOST_LOG_TRIVIAL(error) << "Call: more than 50 Errors";
  }
}

void Call::set_freq(double f) {
  if (f != curr_freq) {
    double position = get_current_length();

    // if there call is being recorded and it isn't the first time the freq is being set
    if (recorder && (freq_count > 0)) {
      Rx_Status rx_status = recorder->get_rx_status();
      freq_list[freq_count - 1].total_len   = rx_status.total_len;
      freq_list[freq_count - 1].spike_count = rx_status.spike_count;
      freq_list[freq_count - 1].error_count = rx_status.error_count;
    }

    Call_Freq call_freq = { f, time(NULL), position };

    if (freq_count < 49) {
      freq_list[freq_count] = call_freq;
      freq_count++;
    } else {
      BOOST_LOG_TRIVIAL(error) << "Call: more than 50 Freq";
    }
    curr_freq = f;
  }
}
int Call::get_sys_num() {
  return sys->get_sys_num();
}
std::string Call::get_short_name() {
  return sys->short_name;
}
long Call::get_talkgroup() {
  return talkgroup;
}

Call_Error * Call::get_error_list() {
  return error_list;
}

long Call::get_error_list_count() {
  return error_list_count;
}

Call_Freq * Call::get_freq_list() {
  return freq_list;
}

long Call::get_freq_count() {
  return freq_count;
}

Call_Source * Call::get_source_list() {
  if (!recorder) {
    BOOST_LOG_TRIVIAL(error) << "Call::get_source_list State is recording, but no recorder assigned!";
  }
  return src_list;
}

long Call::get_source_count() {
  if (!recorder) {
    BOOST_LOG_TRIVIAL(error) << "Call::get_source_count State is recording, but no recorder assigned!";
  }
  return src_count;
}

void Call::set_debug_recording(bool m) {
  debug_recording = m;
}

bool Call::get_debug_recording() {
  return debug_recording;
}

void Call::set_state(State s) {
  state = s;
}

State Call::get_state() {
  return state;
}

void Call::set_encrypted(bool m) {
  encrypted = m;
}

bool Call::get_encrypted() {
  return encrypted;
}

void Call::set_emergency(bool m) {
  emergency = m;
}

bool Call::get_emergency() {
  return emergency;
}

void Call::set_tdma_slot(int m) {
  tdma_slot = m;
  if (!phase2_tdma && tdma_slot) {
    BOOST_LOG_TRIVIAL(error) << "WHAT! SLot is 1 and TDMA is off";
  }
}

int Call::get_tdma_slot() {
  return tdma_slot;
}

void Call::set_phase2_tdma(bool p) {
  phase2_tdma = p;
}

bool Call::get_phase2_tdma() {
  return phase2_tdma;
}

const char * Call::get_xor_mask() {
  return sys->get_xor_mask();
}

bool Call::add_source(long src) {
  if (src == 0) {
    return false;
  }

  double position         = get_current_length();
  Call_Source call_source = { src, time(NULL), position };

  if (src_count < 1) {
    src_list[src_count] = call_source;
    src_count++;
    return true;
  } else if ((src_count < 48) && (src_list[src_count - 1].source != src)) {
    src_list[src_count] = call_source;
    src_count++;
    return true;
  }
  return false;
}

void Call::update(TrunkMessage message) {
  last_update = time(NULL);
  add_source(message.source);
}

int Call::since_last_update() {
  return time(NULL) - last_update;
}

long Call::elapsed() {
  return time(NULL) - start_time;
}

bool Call::is_conventional() {
  return conventional;
}

void Call::set_conventional(bool conv) {
  conventional = conv;
}

int Call::get_idle_count() {
  return idle_count;
}

void Call::reset_idle_count() {
  idle_count = 0;
}

void Call::increase_idle_count() {
  idle_count++;
}

long Call::get_stop_time() {
  return stop_time;
}

long Call::get_start_time() {
  return start_time;
}

char * Call::get_converted_filename() {
  return converted_filename;
}

char * Call::get_filename() {
  return filename;
}
