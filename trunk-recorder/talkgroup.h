#ifndef TALKGROUP_H
#define TALKGROUP_H

#include <iostream>
#include <stdio.h>
#include <string>
//#include <sstream>

class Talkgroup {
public:
  long number;
  unsigned long range;
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

  Talkgroup(int sys_num, long num, unsigned long range, std::string mode, std::string alpha_tag, std::string description, std::string tag, std::string group, int priority, unsigned long preferredNAC);
  Talkgroup(int sys_num, long num, double freq, double tone, std::string alpha_tag, std::string description, std::string tag, std::string group);

  bool is_active();
  int get_priority();
  unsigned long get_preferredNAC();
  void set_priority(int new_priority);
  void set_active(bool a);
  std::string menu_string();

private:
  unsigned long preferredNAC;
  bool active;
};

#endif
