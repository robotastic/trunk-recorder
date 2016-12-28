#ifndef SMARTNET_PARSE_H
#define SMARTNET_PARSE_H
#include "parser.h"
#include <iostream>
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/tokenizer.hpp>


class SmartnetParser:public TrunkParser
{
	int lastcmd;
	long lastaddress;
public:
	SmartnetParser();
	double getfreq(int cmd);
	std::vector<TrunkMessage> parse_message(std::string s);
};
#endif