#ifndef PARSE_H
#define PARSE_H
#include <iostream>
#include <vector>

enum MessageType {
	GRANT = 0,
	STATUS = 1,
	UPDATE = 2,
	CONTROL_CHANNEL = 3,
	UNKNOWN = 99
};

struct TrunkMessage{
	MessageType message_type;
	double freq;
	long talkgroup;
	bool encrypted;
	bool emergency;
	int tdma;
	long source;
};


class TrunkParser {
	std::vector<TrunkMessage> parse_message(std::string s);
};
#endif