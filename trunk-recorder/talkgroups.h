#ifndef TALKGROUPS_H
#define TALKGROUPS_H

#include "talkgroup.h"
#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>

class Talkgroups {
  std::vector<Talkgroup *> talkgroups;

public:
  Talkgroups();
  void load_talkgroups(std::string filename);
  void load_channels(std::string filename);
  Talkgroup *find_talkgroup(long tg);
  Talkgroup *find_talkgroup_by_freq(double freq);
  std::vector<Talkgroup *> get_talkgroups();
};
#endif // TALKGROUPS_H
