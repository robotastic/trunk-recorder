#include "talkgroup.h"


Talkgroup::Talkgroup(long        num,
                     char        m,
                     std::string a,
                     std::string d,
                     std::string t,
                     std::string g,
                     int         p) {
  number      = num;
  mode        = m;
  alpha_tag   = a;
  description = d;
  tag         = t;
  group       = g;
  priority    = p;
  active      = false;
}

std::string Talkgroup::menu_string() {
  char buff[150];

  // std::ostringstream oss;

  sprintf(buff, "%5lu - %-15s %-20s %-15s %-40s", number,
          alpha_tag.c_str(), tag.c_str(), group.c_str(), description.c_str());

  // sprintf(buff, "%5lu - %s", number, alpha_tag.c_str());

  std::string buffAsStdStr = buff;

  return buffAsStdStr;
}

int Talkgroup::get_priority() {
  return priority;
}

bool Talkgroup::is_active() {
  return active;
}

void Talkgroup::set_active(bool a) {
  active = a;
}
