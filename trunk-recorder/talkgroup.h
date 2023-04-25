#ifndef TALKGROUP_H
#define TALKGROUP_H

#include <iostream>
#include <stdio.h>
#include <string>
//#include <sstream>

class Talkgroup {
public:
  Talkgroup(int sys_num, long num, std::string mode, std::string alpha_tag, std::string description, std::string tag, std::string group, int priority, unsigned long preferredNAC);
  Talkgroup(int sys_num, long num, double freq, double tone, std::string alpha_tag, std::string description, std::string tag, std::string group);

  long get_number();
  int get_sys_num();

  bool is_active();
  void set_active(bool new_active);
  
  int get_priority();
  void set_priority(int new_priority);

  unsigned long get_preferredNAC();
  void set_preferredNAC(unsigned long new_preferredNAC);

  std::string get_mode();
  void set_mode(std::string new_mode);

  std::string get_alpha_tag();
  void set_alpha_tag(std::string new_alpha_tag);

  std::string get_description();
  void set_description(std::string new_description);

  std::string get_tag();
  void set_tag(std::string new_tag);

  std::string get_group();
  void set_group(std::string new_group);

  double get_freq();
  void set_freq(double new_freq);

  double get_tone();
  void set_tone(double new_tone);

  std::string menu_string();

private:
  bool active;
  long number;
  std::string mode;
  std::string alpha_tag;
  std::string description;
  std::string tag;
  std::string group;
  int priority;
  int sys_num;
  unsigned long preferredNAC;

  // For Conventional
  double freq;
  double tone;
};

#endif
