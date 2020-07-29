#ifndef SMARTNET_PARSE_H
#define SMARTNET_PARSE_H
#include "parser.h"
#include "system.h"
#include <iostream>
#include <vector>

#include <stdio.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/tokenizer.hpp>

#include <boost/log/trivial.hpp>

#define OSW_BACKGROUND_IDLE     0x2f8
#define OSW_FIRST_CODED_PC      0x304
#define OSW_FIRST_NORMAL        0x308
#define OSW_FIRST_TY2AS1        0x309
#define OSW_EXTENDED_FCN        0x30b
#define OSW_AFFIL_FCN           0x30d
#define OSW_TY2_AFFILIATION     0x310
#define OSW_TY1_STATUS_MIN      0x310
#define OSW_TY2_MESSAGE         0x311
#define OSW_TY1_STATUS_MAX      0x317
#define OSW_TY1_ALERT           0x318
#define OSW_TY1_EMERGENCY       0x319
#define OSW_TY2_CALL_ALERT      0x319
#define OSW_FIRST_ASTRO         0x321
#define OSW_SYSTEM_CLOCK        0x322
#define OSW_SCAN_MARKER         0x32b
#define OSW_EMERG_ANNC          0x32e
#define OSW_AMSS_ID_MIN         0x360
#define OSW_AMSS_ID_MAX         0x39f
#define OSW_CW_ID               0x3a0
#define OSW_SYS_NETSTAT         0x3bf
#define OSW_SYS_STATUS          0x3c0

struct osw_stru{
	unsigned short cmd;
	unsigned short id;
	int status;
	int full_address;
	long address;
	int grp;
};

class SmartnetParser:public TrunkParser
{
	int lastcmd;
	long lastaddress;
public:


	struct osw_stru stack[5];
	short numStacked;
	short numConsumed;
	SmartnetParser();
	double getfreq(int cmd, System *system);
	void print_osw(std::string s);
	bool is_chan(int cmd, System *system);
	std::vector<TrunkMessage> parse_message(std::string s, System *system);
};
#endif
