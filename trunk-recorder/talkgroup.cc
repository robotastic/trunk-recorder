#include "talkgroup.h"

Talkgroup::Talkgroup(int sys_num, long num, std::string mode, std::string alpha_tag, std::string description, std::string tag, std::string group, int priority, unsigned long preferredNAC) {
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
  this->preferredNAC = preferredNAC;
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

long Talkgroup::get_number() {
  return number;
}

int Talkgroup::get_sys_num() {
  return sys_num;
}

bool Talkgroup::is_active() {
  return active;
}

void Talkgroup::set_active(bool new_active) {
  active = new_active;
}

int Talkgroup::get_priority() {
  return priority;
}

void Talkgroup::set_priority(int new_priority) {
  priority = new_priority;
}

unsigned long Talkgroup::get_preferredNAC() {
  return preferredNAC;
}

void Talkgroup::set_preferredNAC(unsigned long new_preferredNAC){
  preferredNAC = new_preferredNAC;
}

std::string Talkgroup::get_mode() {
  return mode;
}

void Talkgroup::set_mode(std::string new_mode) {
  mode = new_mode;
}

std::string Talkgroup::get_alpha_tag() {
  return alpha_tag;
}

void Talkgroup::set_alpha_tag(std::string new_alpha_tag) {
  alpha_tag = new_alpha_tag;
}

std::string Talkgroup::get_description() {
  return description;
}

void Talkgroup::set_description(std::string new_description) {
  description = new_description;
}

std::string Talkgroup::get_tag() {
  return tag;
}

void Talkgroup::set_tag(std::string new_tag) {
  tag = new_tag;
}

std::string Talkgroup::get_group() {
  return group;
}

void Talkgroup::set_group(std::string new_group) {
  group = new_group;
}
  
double Talkgroup::get_freq() {
  return freq;
}

void Talkgroup::set_freq(double new_freq) {
  freq = new_freq;
}

double Talkgroup::get_tone() {
  return tone;
}

void Talkgroup::set_tone(double new_tone) {
  tone = new_tone;
}