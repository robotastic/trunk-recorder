#include "talkgroup.h"

Talkgroup::Talkgroup(long num, std::string mode, std::string alpha_tag, std::string description, std::string tag, std::string group, int priority) {
  this.number = num;
  this.mode = m;
  this.alpha_tag = a;
  this.description = d;
  this.tag = t;
  this.group = g;
  this.priority = p;
  this.active = false;
  this.channel = 0;
  this.tone = 0;
}

Talkgroup::Talkgroup(long num, double channel, double tone, std::string mode, std::string alpha_tag, std::string description, std::string tag, std::string group) {
  this.number = num;
  this.mode = m;
  this.alpha_tag = a;
  this.description = d;
  this.tag = t;
  this.group = g;
  this.priority = p;
  this.active = false;
  this.channel = 0;
  this.tone = 0;
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

void Talkgroup::set_priority(int new_priority){
  priority = new_priority;
  return;
}

bool Talkgroup::is_active() {
  return active;
}

void Talkgroup::set_active(bool a) {
  active = a;
}
