#include "talkgroup.h"

Talkgroup::Talkgroup(int sys_num, long num, unsigned long range, std::string mode, std::string alpha_tag, std::string description, std::string tag, std::string group, int priority, unsigned long preferredNAC) {
  this->sys_num = sys_num;
  this->number = num;
  this->range = range;
  this->mode = mode;
  this->alpha_tag = alpha_tag;
  this->description = description;
  this->tag = tag;
  this->group = group;
  this->priority = priority;
  this->active = false;
  this->freq = 0;
  this->tone = 0;
  this->preferredNAC = preferredNAC;
}

Talkgroup::Talkgroup(int sys_num, long num, double freq, double tone, std::string alpha_tag, std::string description, std::string tag, std::string group) {
  this->sys_num = sys_num;
  this->number = num;
  this->range = 1;
  this->mode = "Z";
  this->alpha_tag = alpha_tag;
  this->description = description;
  this->tag = tag;
  this->group = group;
  this->active = false;
  this->freq = freq;
  this->tone = tone;
  this->priority = 0;
  this->preferredNAC = 0;
}

std::string Talkgroup::menu_string() {
  char buff[150];

  // std::ostringstream oss;

  snprintf(buff, 150, "%5lu - %-15s %-20s %-15s %-40s", number, alpha_tag.c_str(), tag.c_str(), group.c_str(), description.c_str());

  // sprintf(buff, "%5lu - %s", number, alpha_tag.c_str());

  std::string buffAsStdStr = buff;

  return buffAsStdStr;
}

int Talkgroup::get_priority() {
  return priority;
}

unsigned long Talkgroup::get_preferredNAC() {
  return preferredNAC;
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
