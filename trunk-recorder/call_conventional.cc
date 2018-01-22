
#include "formatter.h"
#include <boost/algorithm/string.hpp>
#include "recorders/recorder.h"
#include "call_conventional.h"
#include "./uploaders/stat_socket.h"

Call_conventional::Call_conventional(long t, double f, System *s, Config c, stat_socket * stat_socket) : Call(t,f,s,c) {
  d_stat_socket = stat_socket;
}

Call_conventional::~Call_conventional() {
}

void Call_conventional::restart_call() {
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
    this->update_talkgroup_display();
    recorder->start(this);
}

void Call_conventional::set_recorder(Recorder *r) {
  recorder = r;
  BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\tTG: " << this->get_talkgroup_display() << "\tFreq: " <<  FormatFreq(this->get_freq()) << "\tListening on Src: " << recorder->get_source()->get_device();
}

void Call_conventional::recording_started()
{
  start_time = time(NULL);
  d_stat_socket->send_call_start(this);
}
