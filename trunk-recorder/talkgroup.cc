#include "talkgroup.h"

Talkgroup::Talkgroup(int sys_num, long num, std::string mode, std::string alpha_tag, std::string description, std::string tag, std::string group, int priority, int prefferedNAC) {
  this->sys_num = sys_num;
  this->number = num;
  this->mode = mode;
  this->alpha_tag = alpha_tag;
  this->description = description;
  this->tag = tag;
  this->group = group;
  this->priority = priority;
  this->active = false;
  this->freq = 0;
  this->tone = 0;
  this->prefferedNAC = prefferedNAC;
}

Talkgroup::Talkgroup(int sys_num, long num, double freq, double tone, std::string alpha_tag, std::string description, std::string tag, std::string group) {
  this->sys_num = sys_num;
  this->number = num;
  this->mode = "Z";
  this->alpha_tag = alpha_tag;
  this->description = description;
  this->tag = tag;
  this->group = group;
  this->active = false;
  this->freq = freq;
  this->tone = tone;
  this->priority = 0;
  this->prefferedNAC = 0;
}

std::string Talkgroup::menu_string() {
  char buff[150];

  // std::ostringstream oss;

  sprintf(buff, "%5lu - %-15s %-20s %-15s %-40s", number, alpha_tag.c_str(), tag.c_str(), group.c_str(), description.c_str());

  // sprintf(buff, "%5lu - %s", number, alpha_tag.c_str());

  std::string buffAsStdStr = buff;

  return buffAsStdStr;
}

int Talkgroup::get_priority() {
  return priority;
}

int Talkgroup::get_preferredNAC() {
  return prefferedNAC;
}

void Talkgroup::set_priority(int new_priority) {
  priority = new_priority;
  return;
}

bool Talkgroup::is_active() {
  return active;
}

void Talkgroup::set_active(bool a) {
  active = a;
}
