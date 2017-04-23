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
};
#endif // TALKGROUPS_H
