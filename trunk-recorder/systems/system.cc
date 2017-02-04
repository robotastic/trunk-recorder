#include "system.h"


std::string System::get_api_key() {
  return this->api_key;
}

void System::set_api_key(std::string api_key) {
  this->api_key = api_key;
}

std::string System::get_short_name() {
  return this->short_name;
}

void System::set_short_name(std::string short_name) {
  this->short_name = short_name;
}

std::string System::get_upload_script() {
  return this->upload_script;
}

void System::set_upload_script(std::string script) {
  this->upload_script = script;
}

std::string System::get_default_mode() {
  return this->default_mode;
}

void System::set_default_mode(std::string def_mode) {
  this->default_mode = def_mode;
}

System::System(int sys_id) {
  this->sys_id = sys_id;
  current_control_channel = 0;
}

int System::get_sys_id() {
  return this->sys_id;
}

bool System::get_qpsk_mod() {
  return this->qpsk_mod;
}

void System::set_qpsk_mod(bool qpsk_mod) {
  this->qpsk_mod = qpsk_mod;
}

std::string System::get_system_type() {
  return this->system_type;
}

void System::set_system_type(std::string sys_type) {
  this->system_type = sys_type;
}

std::string System::get_talkgroups_file() {
  return this->talkgroups_file;
}

void System::set_talkgroups_file(std::string talkgroups_file) {
  this->talkgroups_file = talkgroups_file;
}

std::vector<double> System::get_channels(){
  return channels;
}


int System::channel_count() {
  return channels.size();
}

void System::add_conventional_recorder(analog_recorder_sptr rec) {
    conventional_recorders.push_back(rec);
  }
std::vector<analog_recorder_sptr> System::get_conventional_recorders() {
  return conventional_recorders;
}

void System::add_conventionalP25_recorder(p25_recorder_sptr rec) {
	conventionalP25_recorders.push_back(rec);
}

std::vector<p25_recorder_sptr> System::get_conventionalP25_recorders() {
  return conventionalP25_recorders;
}

void System::add_channel(double channel) {
  if (channels.size() == 0) {
    channels.push_back(channel);
  } else {
    if (std::find(channels.begin(), channels.end(), channel) == channels.end()) {
      channels.push_back(channel);
    }
  }
}


int System::control_channel_count() {
  return control_channels.size();
}

void System::add_control_channel(double control_channel) {
  if (control_channels.size() == 0) {
    control_channels.push_back(control_channel);
  } else {
    if (std::find(control_channels.begin(), control_channels.end(),
                  control_channel) == control_channels.end()) {
      control_channels.push_back(control_channel);
    }
  }
}

double System::get_current_control_channel() {
  return this->control_channels[current_control_channel];
}

double System::get_next_control_channel() {
  current_control_channel++;
  if (current_control_channel >= control_channels.size()) {
    current_control_channel = 0;
  }
  return this->control_channels[current_control_channel];
}

void System::set_bandplan(std::string bandplan) {
  this->bandplan = bandplan;
}

std::string System::get_bandplan() {
  return this->bandplan;
}
