
#include "call_conventional.h"
#include "formatter.h"
#include "recorders/recorder.h"
#include <boost/algorithm/string.hpp>

Call_conventional::Call_conventional(long t, double f, System *s, Config c) : Call(t, f, s, c) {
}

Call_conventional::~Call_conventional() {
}

void Call_conventional::restart_call() {
  idle_count = 0;
  freq_count = 0;
  error_list_count = 0;
  curr_src_id = 0;
  start_time = time(NULL);
  stop_time = time(NULL);
  last_update = time(NULL);
  state = RECORDING;
  debug_recording = false;
  phase2_tdma = false;
  tdma_slot = 0;
  encrypted = false;
  emergency = false;
  this->clear_src_list();
  this->create_filename();
  this->update_talkgroup_display();
  recorder->start(this);
}

time_t Call_conventional::get_start_time() {
  // Fixes https://github.com/robotastic/trunk-recorder/issues/103#issuecomment-284825841
  return start_time = stop_time - final_length;
}

void Call_conventional::set_recorder(Recorder *r) {
  recorder = r;
  BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\tTG: " << this->get_talkgroup_display() << "\tFreq: " << FormatFreq(this->get_freq());
}

void Call_conventional::recording_started() {
  start_time = time(NULL);
}
