#ifndef TALKGROUP_H
#define TALKGROUP_H

#include <iostream>
#include <string>
#include <stdio.h>
//#include <sstream>

class Talkgroup {
public:
								long number;
								char mode;
								std::string alpha_tag;
								std::string description;
								std::string tag;
								std::string group;
								int priority;
								Talkgroup(long num, char m, std::string a, std::string d, std::string t, std::string g, int p);
								bool is_active();
								int get_priority();
								void set_active(bool a);
								std::string menu_string();
private:
								bool active;

};

#endif
