#ifndef TALKGROUP_H
#define TALKGROUP_H

#include <iostream>
#include <stdio.h>
#include <string>
//#include <sstream>

class Talkgroup {
public:
  long number;
  std::string mode;
  std::string alpha_tag;
  std::string description;
  std::string tag;
  std::string group;
  int priority;
  int sys_num;

  // For Conventional
  double freq;
  double tone;

  Talkgroup(int sys_num, long num, std::string mode, std::string alpha_tag, std::string description, std::string tag, std::string group, int priority);
  Talkgroup(int sys_num, long num, double freq, double tone, std::string alpha_tag, std::string description, std::string tag, std::string group);

  bool is_active();
  int get_priority();
  void set_priority(int new_priority);
  void set_active(bool a);
  std::string menu_string();

private:
  bool active;
};

#endif
