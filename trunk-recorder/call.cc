#include "call.h"
#include "call_concluder/call_concluder.h"
#include "formatter.h"
#include "recorder_globals.h"
#include "recorders/recorder.h"
#include "source.h"
#include <boost/algorithm/string.hpp>
#include <signal.h>
#include <stdio.h>

std::string Call::get_capture_dir() {
  return this->config.capture_dir;
}

Call::Call(long t, double f, System *s, Config c) {
  config = c;
  call_num = call_counter++;
  idle_count = 0;
  freq_count = 0;
  error_list_count = 0;
  curr_freq = 0;
  curr_src_id = 0;
  talkgroup = t;
  sys = s;
  start_time = time(NULL);
  stop_time = time(NULL);
  last_update = time(NULL);
  state = MONITORING;
  debug_recording = false;
  sigmf_recording = false;
  recorder = NULL;
  phase2_tdma = false;
  tdma_slot = 0;
  encrypted = false;
  emergency = false;
  duplex = false;
  mode = false;
  priority = 0;
  set_freq(f);
  this->update_talkgroup_display();
}

Call::Call(TrunkMessage message, System *s, Config c) {
  config = c;
  call_num = call_counter++;
  idle_count = 0;
  freq_count = 0;
  error_list_count = 0;
  curr_src_id = 0;
  curr_freq = 0;
  talkgroup = message.talkgroup;
  sys = s;
  start_time = time(NULL);
  stop_time = time(NULL);
  last_update = time(NULL);
  state = MONITORING;
  debug_recording = false;
  sigmf_recording = false;
  recorder = NULL;
  phase2_tdma = message.phase2_tdma;
  tdma_slot = message.tdma_slot;
  encrypted = message.encrypted;
  emergency = message.emergency;
  duplex = message.duplex;
  mode = message.mode;
  priority = message.priority;
  set_freq(message.freq);
  add_source(message.source);
  this->update_talkgroup_display();
}

Call::~Call() {
  //  BOOST_LOG_TRIVIAL(info) << " This call is over!!";
}

void Call::restart_call() {
}

void Call::stop_call() {

  if (this->get_recorder() != NULL) {
    // If the call is being recorded, check to see if the recorder is currently in an INACTIVE state. This means that the recorder is not
    // doing anything and can be stopped.
    if ((state == RECORDING) && this->get_recorder()->is_idle()) {
      BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\t\033[0;34m" << this->get_call_num() << "C\033[0m\tTG: " << this->get_talkgroup_display() << "\tFreq: " << format_freq(get_freq()) << "\tStopping Recorded Call, setting call state to COMPLETED - Last Update: " << this->since_last_update() << "s";
      this->set_state(COMPLETED);
    } else {
      BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\t\033[0;34m" << this->get_call_num() << "C\033[0m\tTG: " << this->get_talkgroup_display() << "\tFreq: " << format_freq(get_freq()) << "\tStopping Recorded Call, setting call state to INACTIVE - Last Update: " << this->since_last_update() << "s";
      this->set_state(INACTIVE);
    }
    this->get_recorder()->set_record_more_transmissions(false);
  }
}
long Call::get_call_num() {
  return call_num;
}
void Call::conclude_call() {

  //BOOST_LOG_TRIVIAL(info) << "conclude_call()";
  stop_time = time(NULL);

  if (state == COMPLETED) {
    final_length = recorder->get_current_length();

    if (freq_count > 0) {
      Rx_Status rx_status = recorder->get_rx_status();
      if (rx_status.last_update > 0)
        stop_time = rx_status.last_update;
      freq_list[freq_count - 1].total_len = rx_status.total_len;
      freq_list[freq_count - 1].spike_count = rx_status.spike_count;
      freq_list[freq_count - 1].error_count = rx_status.error_count;
    }
    if (!recorder) {
      BOOST_LOG_TRIVIAL(error) << "Call::end_call() State is recording, but no recorder assigned!";
    }
    BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\t\033[0;34m" << this->get_call_num() << "C\033[0m\tTG: " << this->get_talkgroup_display() << "\tFreq: " << format_freq(get_freq()) << "\t\u001b[33mConcluding Recorded Call\u001b[0m - Last Update: " << this->since_last_update() << "s\tCall Elapsed: " << this->elapsed();

    this->get_recorder()->stop();
    transmission_list = this->get_recorder()->get_transmission_list();
    if (this->get_sigmf_recording() == true) {
      this->get_sigmf_recorder()->stop();
    }

    if (this->get_debug_recording() == true) {
      this->get_debug_recorder()->stop();
    }

    Call_Concluder::conclude_call(this, sys, config);
  } else {
    BOOST_LOG_TRIVIAL(error) << "[" << sys->get_short_name() << "]\t\033[0;34m" << this->get_call_num() << "C\033[0m\tTG: " << this->get_talkgroup_display() << "Concluding call, but call state is not COMPLETED!";
  }
}

void Call::set_sigmf_recorder(Recorder *r) {
  sigmf_recorder = r;
}

Recorder *Call::get_sigmf_recorder() {
  return sigmf_recorder;
}

void Call::set_debug_recorder(Recorder *r) {
  debug_recorder = r;
}

Recorder *Call::get_debug_recorder() {
  return debug_recorder;
}

void Call::set_recorder(Recorder *r) {
  recorder = r;
  //BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\t\033[0;34m" << this->get_call_num() << "C\033[0m\tTG: " << this->get_talkgroup_display() << "\tFreq: " << format_freq(this->get_freq()) << "\t\u001b[32mStarting Recorder on Src: " << recorder->get_source()->get_device() << "\u001b[0m";
}

Recorder *Call::get_recorder() {
  return recorder;
}

double Call::get_freq() {
  return curr_freq;
}

double Call::get_final_length() {
  return final_length;
}

double Call::get_current_length() {
  if ((state == RECORDING) && recorder) {
    if (!recorder) {
      BOOST_LOG_TRIVIAL(error) << "Call::get_current_length() State is recording, but no recorder assigned!";
    }
    return get_recorder()->get_current_length(); // This could SegFault
  } else {
    return time(NULL) - start_time;
  }
}

void Call::set_error(Rx_Status rx_status) {
  Call_Error call_error = {curr_freq, rx_status.total_len, rx_status.error_count, rx_status.spike_count};

  if (error_list_count < 49) {
    error_list[error_list_count] = call_error;
    error_list_count++;
  } else {
    BOOST_LOG_TRIVIAL(error) << "Call: more than 50 Errors";
  }
}

System *Call::get_system() {
  return sys;
}

void Call::set_freq(double f) {
  if (f != curr_freq) {
    double position = get_current_length();

    // if there call is being recorded and it isn't the first time the freq is being set
    if (recorder && (freq_count > 0)) {
      Rx_Status rx_status = recorder->get_rx_status();
      freq_list[freq_count - 1].total_len = rx_status.total_len;
      freq_list[freq_count - 1].spike_count = rx_status.spike_count;
      freq_list[freq_count - 1].error_count = rx_status.error_count;
    }

    Call_Freq call_freq = {f, time(NULL), position};

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

Call_Error *Call::get_error_list() {
  return error_list;
}

long Call::get_error_list_count() {
  return error_list_count;
}

Call_Freq *Call::get_freq_list() {
  return freq_list;
}

long Call::get_freq_count() {
  return freq_count;
}

void Call::clear_transmission_list() {
  transmission_list.clear();
  transmission_list.shrink_to_fit();
}

void Call::set_debug_recording(bool m) {
  debug_recording = m;
}

bool Call::get_debug_recording() {
  return debug_recording;
}

void Call::set_sigmf_recording(bool m) {
  sigmf_recording = m;
}

bool Call::get_sigmf_recording() {
  return sigmf_recording;
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

const char *Call::get_xor_mask() {
  return sys->get_xor_mask();
}

long Call::get_current_source_id() {
  return curr_src_id;
}

bool Call::add_source(long src) {
  if (src == 0) {
    return false;
  }

  if (src == curr_src_id) {
    return false;
  }

  curr_src_id = src;

  if (state == RECORDING) {
     Recorder *rec = this->get_recorder(); 
    if (rec != NULL) {
      rec->set_source(src);
    }
  }

  plugman_signal(src, NULL, gr::blocks::SignalType::Normal, this, this->get_system(), NULL);

  return true ; //add_signal_source(src, NULL, gr::blocks::SignalType::Normal);
}

bool Call::update(TrunkMessage message) {
  if ((state == INACTIVE) || (state == COMPLETED)) {
    BOOST_LOG_TRIVIAL(error) << "[" << sys->get_short_name() << "]\t\033[0;34m" << this->get_call_num() << "C\033[0m\tCall Update, but state is: " << state << " - Call TG: " << get_talkgroup() << "\t Call Freq: " << get_freq() << "\tMsg Tg: " << message.talkgroup << "\tMsg Freq: " << message.freq;
  } else {
    last_update = time(NULL);
    if ((message.freq != this->curr_freq) || (message.talkgroup != this->talkgroup)) {
      BOOST_LOG_TRIVIAL(error) << "[" << sys->get_short_name() << "]\t\033[0;34m" << this->get_call_num() << "C\033[0m\tCall Update, message mismatch - Call TG: " << get_talkgroup() << "\t Call Freq: " << get_freq() << "\tMsg Tg: " << message.talkgroup << "\tMsg Freq: " << message.freq;
    } else {
      return add_source(message.source);
    }
  }
  return false;
}

int Call::since_last_update() {
  /*long last_rx;
  if (get_recorder() && (last_rx = recorder->get_rx_status().last_update)) {
    BOOST_LOG_TRIVIAL(trace) << "temp.last_update: " << last_rx << " diff: " << time(NULL) - last_rx;
    return time(NULL) - last_rx;
    //last_update = temp.last_update;
  } else {*/
    BOOST_LOG_TRIVIAL(trace) << "last_update: " << last_update << " diff: " << time(NULL) - last_update;
    return time(NULL) - last_update;
  //}
}

long Call::elapsed() {
  return time(NULL) - start_time;
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

std::string Call::get_system_type() {
  return sys->get_system_type().c_str();
}

void Call::set_talkgroup_tag(std::string tag) {
  talkgroup_tag = tag;
  update_talkgroup_display();
}

std::string Call::get_talkgroup_display() {
  return talkgroup_display;
}

std::string Call::get_talkgroup_tag() {
  return talkgroup_tag;
}

bool Call::get_conversation_mode() {
  if (!sys) {
    BOOST_LOG_TRIVIAL(error) << "\tWEIRD! for some reason, call has no sys - Call TG: " << get_talkgroup() << "\t Call Freq: " << get_freq();
    return false;
  }
  return sys->get_conversation_mode();
}

void Call::update_talkgroup_display() {
  boost::trim(talkgroup_tag);
  if (talkgroup_tag.empty()) {
    talkgroup_tag = "-";
  }

  char formattedTalkgroup[62];
  if (this->sys->get_talkgroup_display_format() == System::talkGroupDisplayFormat_id_tag) {
    snprintf(formattedTalkgroup, 61, "%10ld (%c[%dm%23s%c[0m)", talkgroup, 0x1B, 35, talkgroup_tag.c_str(), 0x1B);
  } else if (this->sys->get_talkgroup_display_format() == System::talkGroupDisplayFormat_tag_id) {
    snprintf(formattedTalkgroup, 61, "%c[%dm%23s%c[0m (%10ld)", 0x1B, 35, talkgroup_tag.c_str(), 0x1B, talkgroup);
  } else {
    snprintf(formattedTalkgroup, 61, "%c[%dm%10ld%c[0m", 0x1B, 35, talkgroup, 0x1B);
  }
  talkgroup_display = boost::lexical_cast<std::string>(formattedTalkgroup);
}

boost::property_tree::ptree Call::get_stats() {
  boost::property_tree::ptree call_node;
  boost::property_tree::ptree freq_list_node;
  boost::property_tree::ptree source_list_node;
  call_node.put("id", boost::lexical_cast<std::string>(this->get_sys_num()) + "_" + boost::lexical_cast<std::string>(this->get_talkgroup()) + "_" + boost::lexical_cast<std::string>(this->get_start_time()));
  call_node.put("freq", this->get_freq());
  call_node.put("sysNum", this->get_sys_num());
  call_node.put("shortName", this->get_short_name());
  call_node.put("talkgroup", this->get_talkgroup());
  call_node.put("talkgrouptag", this->get_talkgroup_tag());
  call_node.put("elasped", this->elapsed());
  if (get_state() == RECORDING)
    call_node.put("length", this->get_current_length());
  else
    call_node.put("length", this->get_final_length());
  call_node.put("state", this->get_state());
  call_node.put("phase2", this->get_phase2_tdma());
  call_node.put("conventional", this->is_conventional());
  call_node.put("encrypted", this->get_encrypted());
  call_node.put("emergency", this->get_emergency());
  call_node.put("startTime", this->get_start_time());
  call_node.put("stopTime", this->get_stop_time());
  call_node.put("srcId", this->get_current_source_id());


  Recorder *recorder = this->get_recorder();

  if (recorder) {
    call_node.put("recNum", recorder->get_num());
    call_node.put("srcNum", recorder->get_source()->get_num());
    call_node.put("recState", recorder->get_state());
    call_node.put("analog", recorder->is_analog());
  }


  return call_node;
}

long Call::call_counter = 0;