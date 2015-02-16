#include "smartnet_parser.h"

using namespace std;
SmartnetParser::SmartnetParser() {
	lastaddress = 0;
	lastcmd = 0;
}
double SmartnetParser::getfreq(int cmd) {
	float freq;
	if (cmd < 0x1b8) {
		freq = float(cmd * 0.025 + 851.0125);
	} else if (cmd < 0x230) {
		freq = float(cmd * 0.025 + 851.0125 - 10.9875);
	} else {
		freq = 0;
	}

	return freq*1000000;
}



std::vector<TrunkMessage> SmartnetParser::parse_message(std::string s) {
	std::vector<TrunkMessage> messages;
	TrunkMessage message;


	message.message_type = UNKNOWN;


	std::vector<std::string> x;
	boost::split(x, s, boost::is_any_of(","), boost::token_compress_on);

	long address = atoi( x[0].c_str() ) & 0xFFF0;
	//int groupflag = atoi( x[1].c_str() );
	int command = atoi( x[2].c_str() );

	x.clear();
	vector<string>().swap(x);

	if (command < 0x2d0) {
		if (  (address != 56016) && (address != 8176)) {  // remove this later to make it more general
			message.talkgroup = address;
			message.freq = getfreq(command);
			if ( lastcmd == 0x308) {
				// Channel Grant
				message.message_type = GRANT;
			} else {
				// Call continuation
				message.message_type = UPDATE;
			}
		}
	}


	if (command == 0x03c0) {
		message.message_type = STATUS;
		//parse_status(command, address,groupflag);
	}

	lastaddress = address;
	lastcmd = command;
	messages.push_back(message);
	return messages;
}