#include "call.h"

void Call::create_filename() {
  tm *ltm = localtime(&start_time);


  std::stringstream path_stream;

  path_stream << this->config.capture_dir << "/" << sys->get_short_name() << "/" << 1900 + ltm->tm_year << "/" <<  1 + ltm->tm_mon << "/" << ltm->tm_mday;

  boost::filesystem::create_directories(path_stream.str());
  sprintf(filename,        "%s/%ld-%ld_%g.wav",
          path_stream.str().c_str(), talkgroup, start_time, curr_freq);
  sprintf(status_filename, "%s/%ld-%ld_%g.json",
          path_stream.str().c_str(), talkgroup, start_time, curr_freq);

  // sprintf(filename, "%s/%ld-%ld.wav",
  // path_stream.str().c_str(),talkgroup,start_time);
  // sprintf(status_filename, "%s/%ld-%ld.json",
  // path_stream.str().c_str(),talkgroup,start_time);
}

Call::Call(long t, double f, System *s, Config c) {
  config          = c;
    freq_count = 0;
    curr_freq = 0;
    set_freq(f);
  talkgroup       = t;
  sys         = s;
  start_time      = time(NULL);
    stop_time      = time(NULL);
  last_update     = time(NULL);
  state           = monitoring;
  debug_recording = false;
  recorder        = NULL;
  tdma            = false;
  encrypted       = false;
  emergency       = false;

  this->create_filename();
}

Call::Call(TrunkMessage message, System *s, Config c) {
  config          = c;
    freq_count = 0;
    curr_freq = 0;
    set_freq(message.freq);

  talkgroup       = message.talkgroup;
  sys          = s;
  start_time      = time(NULL);

  stop_time      = time(NULL);
  last_update     = time(NULL);
  state           = monitoring;
  debug_recording = false;
  recorder        = NULL;
  tdma            = message.tdma;
  encrypted       = message.encrypted;
  emergency       = message.emergency;

  this->create_filename();
}

Call::~Call() {

  //  BOOST_LOG_TRIVIAL(info) << " This call is over!!";
}

void Call::end_call() {

    stop_time      = time(NULL);
  if (state == recording) {
    BOOST_LOG_TRIVIAL(info) << "Ending Recorded Call \tTG: " <<   this->get_talkgroup() << "\tLast Update: " << this->since_last_update() << " Call Elapsed: " << this->elapsed();
    std::ofstream myfile(status_filename);
    std::stringstream shell_command;
    Call_Source *wav_src_list = get_recorder()->get_source_list();
    int wav_src_count = get_recorder()->get_source_count();

    if (myfile.is_open())
    {
      myfile << "{\n";
      myfile << "\"freq\": " << this->curr_freq << ",\n";
      myfile << "\"start_time\": " << this->start_time << ",\n";
      myfile << "\"stop_time\": " << this->stop_time << ",\n";
      myfile << "\"emergency\": " << this->emergency << ",\n";
      myfile << "\"talkgroup\": " << this->talkgroup << ",\n";
      myfile << "\"srcList\": [ ";

      for (int i = 0; i < wav_src_count; i++) {
        if (i != 0) {
          myfile << ", " <<  wav_src_list[i].source;
        } else {
          myfile << wav_src_list[i].source;
        }
      }
      myfile << " ]\n";
      myfile << "\"freqList\": [ ";

      for (int i = 0; i < freq_count; i++) {
          if (i != 0) {
          myfile << ", ";
        }
          myfile << "{ \"freq:\" " <<  freq_list[i].freq <<", \"time:\" " << freq_list[i].time << ", \"pos:\" " << freq_list[i].position << "}";
      }
      myfile << " ]\n";
      myfile << "}\n";
      myfile.close();
    }
    if (sys->get_upload_script().length() != 0) {
      shell_command << "./"<< sys->get_upload_script() << " " << this->get_filename() << " &";
    }
    this->get_recorder()->stop();
    if (this->config.upload_server != "") {
      send_call(this, sys, config);
    } else {

    }
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

void Call::set_freq(double f) {
  if (f!=curr_freq){
    long position;
      if (state == recording) {
        position = get_recorder()->get_current_length();
      } else {
        position = time(NULL) - start_time;
      }
      Call_Freq call_freq = { f, time(NULL), position };
      freq_list[freq_count] = call_freq;
      freq_count++;
      curr_freq = f;
  }

}

long Call::get_talkgroup() {
  return talkgroup;
}

Call_Freq * Call::get_freq_list() {
  return freq_list;
}

long Call::get_freq_count() {
return freq_count;
}
Call_Source * Call::get_source_list() {
  return get_recorder()->get_source_list();
}

long Call::get_source_count() {
  return get_recorder()->get_source_count();
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

void Call::set_tdma(int m) {
  tdma = m;
}

int Call::get_tdma() {
  return tdma;
}

void Call::update(TrunkMessage message) {
  last_update = time(NULL);
}

int Call::since_last_update() {
  return time(NULL) - last_update;
}

long Call::elapsed() {
  return time(NULL) - start_time;
}

long Call::get_stop_time() {
  return stop_time;
}
long Call::get_start_time() {
  return start_time;
}

char * Call::get_filename() {
  return filename;
}
