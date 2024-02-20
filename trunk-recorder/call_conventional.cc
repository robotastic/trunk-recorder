
#include "call_conventional.h"
#include "formatter.h"
#include "recorders/recorder.h"
#include <boost/algorithm/string.hpp>

Call_conventional::Call_conventional(long t, double f, System *s, Config c) : Call_impl(t, f, s, c) {
}

void Call_conventional::restart_call() {
  idle_count = 0;
  curr_src_id = -1;
  start_time = time(NULL);
  stop_time = time(NULL);
  last_update = time(NULL);
  state = RECORDING;
  debug_recording = false;
  phase2_tdma = false;
  tdma_slot = 0;
  encrypted = false;
  emergency = false;
  this->update_talkgroup_display();
  recorder->start(this);
  recroder->set_rssi(0);
}

time_t Call_conventional::get_start_time() {
  // Fixes https://github.com/robotastic/trunk-recorder/issues/103#issuecomment-284825841
  return start_time = stop_time - final_length;
}

void Call_conventional::set_recorder(Recorder *r) {
  recorder = r;
  BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\tTG: " << this->get_talkgroup_display() << "\tFreq: " << format_freq(this->get_freq());
}

void Call_conventional::recording_started() {
  start_time = time(NULL);
}
