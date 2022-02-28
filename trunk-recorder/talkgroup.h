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

  // For Conventional
  double channel;
  double tone;

  Talkgroup(long num, double channel, double tone, std::string mode, std::string alpha_tag, std::string description, std::string tag, std::string group);

  Talkgroup(long num, std::string m, std::string a, std::string d, std::string t, std::string g, int p);
  bool is_active();
  int get_priority();
  void set_priority(int new_priority);
  void set_active(bool a);
  std::string menu_string();

private:
  bool active;
};

#endif
