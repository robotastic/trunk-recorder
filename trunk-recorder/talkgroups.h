#ifndef TALKGROUPS_H
#define TALKGROUPS_H

#include "talkgroup.h"

#include <string>
#include <vector>

class Talkgroups {
  std::vector<Talkgroup *> talkgroups;

public:
  Talkgroups();
  void load_talkgroups(std::string filename);
  Talkgroup *find_talkgroup(long tg);
  void add(long num, std::string alphaTag);
};
#endif // TALKGROUPS_H
