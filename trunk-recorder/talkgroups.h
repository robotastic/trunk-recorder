#ifndef TALKGROUPS_H
#define TALKGROUPS_H
#include "talkgroup.h"

#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/tokenizer.hpp>
#include <boost/intrusive_ptr.hpp>

class Talkgroups {
	std::vector<Talkgroup *> talkgroups;
public:
	Talkgroups();
	void load_talkgroups(std::string filename);
	Talkgroup *find_talkgroup(long tg);
};
#endif