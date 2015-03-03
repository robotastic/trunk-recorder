
#define DSD
//#include <tuple>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <boost/foreach.hpp>
#include <cassert>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ncurses.h>
#include <menu.h>
#include <time.h>
#include "recorder.h"

#ifdef DSD
#include "dsd_recorder.h"
#endif

#include "p25_recorder.h"


#include "analog_recorder.h"
#include "smartnet_trunking.h"
#include "p25_trunking.h"
#include "smartnet_crc.h"
#include "smartnet_deinterleave.h"
#include "talkgroups.h"
#include "source.h"
#include "call.h"
#include "smartnet_parser.h"
#include "p25_parser.h"
#include "parser.h"

#include <osmosdr/source.h>
#include <gnuradio/uhd/usrp_source.h>


#include <boost/program_options.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/tokenizer.hpp>
#include <boost/intrusive_ptr.hpp>

#include <gnuradio/msg_queue.h>
#include <gnuradio/message.h>
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/gr_complex.h>

#include <gnuradio/top_block.h>


using namespace std;

std::vector<Source *> sources;
std::vector<double> control_channels;
std::map<long,long> unit_affiliations;
int current_control_channel = 0;
std::vector<Call *> calls;
Talkgroups *talkgroups;
std::string talkgroups_file;
string system_type;
gr::top_block_sptr tb;
smartnet_trunking_sptr smartnet_trunking;
p25_trunking_sptr p25_trunking;
gr::msg_queue::sptr queue;

volatile sig_atomic_t exit_flag = 0;
SmartnetParser *smartnet_parser;
P25Parser *p25_parser;


void exit_interupt(int sig) { // can be called asynchronously
	exit_flag = 1; // set flag
}

unsigned GCD(unsigned u, unsigned v) {
	while ( v != 0) {
		unsigned r = u % v;
		u = v;
		v = r;
	}
	return u;
}

std::vector<float> design_filter(double interpolation, double deci) {
	float beta = 5.0;
	float trans_width = 0.5 - 0.4;
	float mid_transition_band = 0.5 - trans_width/2;

	std::vector<float> result = gr::filter::firdes::low_pass(
	                                interpolation,
	                                1,
	                                mid_transition_band/interpolation,
	                                trans_width/interpolation,
	                                gr::filter::firdes::WIN_KAISER,
	                                beta
	                            );

	return result;
}


void load_config()
{

	try
	{

		const std::string json_filename = "config.json";

		boost::property_tree::ptree pt;
		boost::property_tree::read_json(json_filename, pt);

		BOOST_FOREACH( boost::property_tree::ptree::value_type  &node,pt.get_child("sources") )
		{
			double center = node.second.get<double>("center",0);
			double rate = node.second.get<double>("rate",0);
			double error = node.second.get<double>("error",0);
			int gain = node.second.get<int>("gain",0);
			int if_gain = node.second.get<int>("ifGain",0);
			int bb_gain = node.second.get<int>("bbGain",0);
			std::string antenna = node.second.get<string>("antenna","");
			int digital_recorders = node.second.get<int>("digitalRecorders",0);
			int analog_recorders = node.second.get<int>("analogRecorders",0);

			std::string driver = node.second.get<std::string>("driver","");
			std::string device = node.second.get<std::string>("device","");

			std::cout << "Center: " << node.second.get<double>("center",0) << std::endl;
			std::cout << "Rate: " << node.second.get<double>("rate",0) << std::endl;
			std::cout << "Error: " << node.second.get<double>("error",0) << std::endl;
			std::cout << "Gain: " << node.second.get<int>("gain",0) << std::endl;
			std::cout << "IF Gain: " << node.second.get<int>("ifGain",0) << std::endl;
			std::cout << "BB Gain: " << node.second.get<int>("bbGain",0) << std::endl;

			std::cout << "Digital Recorders: " << node.second.get<int>("digitalRecorders",0) << std::endl;
			std::cout << "Analog Recorders: " << node.second.get<int>("analogRecorders",0) << std::endl;
			std::cout << "driver: " << node.second.get<std::string>("driver","") << std::endl;


			Source *source = new Source(center,rate,error,driver,device);
			std::cout << "Max HZ: " << source->get_max_hz() << std::endl;
			std::cout << "Min HZ: " << source->get_min_hz() << std::endl;
			source->set_if_gain(if_gain);
			source->set_bb_gain(bb_gain);
			source->set_gain(gain);
			source->set_antenna(antenna);
			source->create_digital_recorders(tb, digital_recorders);
			source->create_analog_recorders(tb, analog_recorders);
			sources.push_back(source);
		}

		BOOST_FOREACH( boost::property_tree::ptree::value_type  &node,pt.get_child("system.control_channels") )
		{
			double control_channel = node.second.get<double>("",0);
			control_channels.push_back(control_channel);
			std::cout << node.second.get<double>("",0) << std::endl;

		}

		talkgroups_file = pt.get<std::string>("talkgroupsFile","");
		std::cout << "Talkgroups File: " << talkgroups_file << std::endl;
		system_type = pt.get<std::string>("system.type");

	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
	}

}



void start_recorder(TrunkMessage message) {
	Call * call = new Call(message);
	Talkgroup * talkgroup = talkgroups->find_talkgroup(message.talkgroup);
	bool source_found = false;
	Recorder *recorder;
	call->set_recording(false); // start with the assumption that there are no recorders available.
	std::cout << "\tCall created for: " << call->get_talkgroup() << "\tTDMA: " << call->get_tdma() <<  "\tEncrypted: " << call->get_encrypted() << std::endl;

	if (message.encrypted == false) {
		for(vector<Source *>::iterator it = sources.begin(); it != sources.end(); it++) {
			Source * source = *it;

			if ((source->get_min_hz() <= message.freq) && (source->get_max_hz() >= message.freq)) {
				source_found = true;
				if (talkgroup)
				{
					if (talkgroup->mode == 'A') {
						recorder = source->get_analog_recorder(talkgroup->get_priority());
					} else {
						recorder = source->get_digital_recorder(talkgroup->get_priority());
					}
				} else {
					recorder = source->get_digital_recorder(3);
				}
				if (recorder) {
					recorder->activate( message.talkgroup,message.freq, calls.size());
					call->set_recorder(recorder);
					call->set_recording(true);
				} else {
					//std::cout << "\tNot recording call" << std::endl;
				}


			}

		}
		if (!source_found) {
			std::cout << "\tRecording not started because there was no source covering: " << message.freq << std::endl;
		}
	}

	calls.push_back(call);
}
void stop_inactive_recorders() {

	char shell_command[200];

	for(vector<Call *>::iterator it = calls.begin(); it != calls.end();) {
		Call *call = *it;
		if ( call->since_last_update()  >= 5.0) {


			if (call->get_recording() == true) {
				sprintf(shell_command,"./encode-upload.sh %s > /dev/null 2>&1 &", call->get_recorder()->get_filename());
				call->get_recorder()->deactivate();
				system(shell_command);
			}

			std::cout << "\tRemoving TG: " << call->get_talkgroup() << "\tElapsed: " << call->elapsed() << std::endl;
			it = calls.erase(it);
		} else {
			++it;
		}//if rx is active
	}//foreach loggers
}

void assign_recorder(TrunkMessage message) {
	bool call_found = false;
	char shell_command[200];

	for(vector<Call *>::iterator it = calls.begin(); it != calls.end();) {
		Call *call= *it;


		if (call->get_talkgroup() == message.talkgroup) {
			if (call->get_freq() != message.freq) {
				std::cout << "\tRetune - Total calls: " << calls.size() << "\tTalkgroup: " << message.talkgroup << "\tOld Freq: " << call->get_freq() << "\tNew Freq: " << message.freq << std::endl;
				// not sure what to do here; looks like we should retune
				call->set_freq(message.freq);
				call->set_tdma(message.tdma);
				if (call->get_recording() == true) {
					call->get_recorder()->tune_offset(message.freq);

				}
			}
			call->update();
			call_found = true;

			++it; // move on to the next one
		} else {

			if ((call->get_freq() == message.freq) && (call->get_tdma() == message.tdma)) {
				//call_found = true;

				std::cout << "\tFreq in use - Update for TG: " << message.talkgroup << "\tFreq: " << message.freq << "\tTDMA: " << message.tdma << "\tExisting call\tTG: " << call->get_talkgroup() << "\tTMDA: " << call->get_tdma() << "\tElapsed: " << call->elapsed() << std::endl;
				//different talkgroups on the same freq, that is trouble

				if (call->get_recording() == true) {
					sprintf(shell_command,"./encode-upload.sh %s > /dev/null 2>&1 &", call->get_recorder()->get_filename());
					call->get_recorder()->deactivate();
					system(shell_command);
				}
				it = calls.erase(it);
			} else {
				++it; // move on to the next one
			}
		}
	}


	if (!call_found) {

		start_recorder(message);
	}
}


void add_control_channel(double control_channel) {
	if (std::find(control_channels.begin(), control_channels.end(), control_channel) != control_channels.end()) {
		control_channels.push_back(control_channel);
	}
}



void unit_registration(long unit) {

}

void unit_deregistration(long unit) {
	std::map<long, long>::iterator it;

	it = unit_affiliations.find (unit);   
	if (it != unit_affiliations.end()) {
    unit_affiliations.erase(it);
	}
}

void group_affiliation(long unit, long talkgroup) {
	unit_affiliations[unit] = talkgroup;
}

void update_recorder(TrunkMessage message) {

	for(vector<Call *>::iterator it = calls.begin(); it != calls.end(); ++it) {
		Call *call= *it;


		if (call->get_talkgroup() == message.talkgroup) {
			if (call->get_freq() != message.freq) {
				std::cout << "\tUpdate Retune - Total calls: " << calls.size() << "\tTalkgroup: " << message.talkgroup << "\tOld Freq: " << call->get_freq() << "\tNew Freq: " << message.freq << std::endl;
				// not sure what to do here; looks like we should retune
				call->set_freq(message.freq);
				call->set_tdma(message.tdma);
				if (call->get_recording() == true) {
					call->get_recorder()->tune_offset(message.freq);
				}
			}
			call->update();
		}
	}
}

void unit_check() {
	std::map<long, long> talkgroup_totals;
	std::map<long, long>::iterator it;

	char unit_filename[160];

	
	sprintf(unit_filename, "unit_check.json");

	for(it = unit_affiliations.begin(); it != unit_affiliations.end(); ++it) {
			talkgroup_totals[it->second]++;
	}

		ofstream myfile (unit_filename);
	if (myfile.is_open())
	{
		myfile << "{\n";
		myfile << "talkgroups: [\n";
		for(it = talkgroup_totals.begin(); it != talkgroup_totals.end(); ++it) {
			talkgroup_totals[it->second]++;
			myfile << it->first << ": " << it->second;
			if (it != talkgroup_totals.end()) {
				myfile << ",";
			}
			myfile << "\n";
		}
		myfile << "]\n}\n";
		system("./unit_check.sh > /dev/null 2>&1 &");
		myfile.close();
	}
}

void handle_message(std::vector<TrunkMessage>  messages) {
	for(std::vector<TrunkMessage>::iterator it = messages.begin(); it != messages.end(); it++) {
		TrunkMessage message = *it;

		switch(message.message_type) {
		case GRANT:
			assign_recorder(message);
			break;
		case UPDATE:
			update_recorder(message);
			break;
		case CONTROL_CHANNEL:
			add_control_channel(message.freq);
			break;
		case REGISTRATION:
			break;
		case DEREGISTRATION:
			unit_deregistration(message.source);
			break;
		case AFFILIATION:
			group_affiliation(message.source, message.talkgroup);
			break; 
		case STATUS:
		case UNKNOWN:
			break;
		}
	}
}

void monitor_messages() {
	gr::message::sptr msg;
	int messagesDecodedSinceLastReport = 0;
	float msgs_decoded_per_second = 0;

	time_t lastMsgCountTime = time(NULL);;
	time_t lastTalkgroupPurge = time(NULL);;
	time_t currentTime = time(NULL);
	time_t lastUnitCheckTime = time(NULL);
	std::vector<TrunkMessage> trunk_messages;

	while (1) {
		if(exit_flag) { // my action when signal set it 1
			printf("\n Signal caught!\n");
			return;
		}


		msg = queue->delete_head();
		messagesDecodedSinceLastReport++;
		currentTime = time(NULL);
		

		if ((currentTime - lastTalkgroupPurge) >= 1.0 )
		{
			stop_inactive_recorders();
			lastTalkgroupPurge = currentTime;
		}
		if (system_type == "smartnet") {
			trunk_messages = smartnet_parser->parse_message(msg->to_string());
		}
		if (system_type == "p25") {
			trunk_messages = p25_parser->parse_message(msg);
		}
		else {
			std::cout << msg->to_string() << std::endl;
		}
		handle_message(trunk_messages);

		float timeDiff = currentTime - lastMsgCountTime;
		if (timeDiff >= 3.0) {
			msgs_decoded_per_second = messagesDecodedSinceLastReport/timeDiff;
			messagesDecodedSinceLastReport = 0;
			lastMsgCountTime = currentTime;
			if (msgs_decoded_per_second < 10 ) {
				std::cout << "\tControl Channel Message Decode Rate: " << msgs_decoded_per_second << "/sec" << std::endl;
			}
		}

		
		if ((currentTime - lastUnitCheckTime) >= 300.0) {
			unit_check();
			lastUnitCheckTime = currentTime;
		}

		msg.reset();



	}
}


bool monitor_system() {
	bool source_found = false;
	Source * source = NULL;
	double control_channel_freq = control_channels[current_control_channel];

	for(vector<Source *>::iterator it = sources.begin(); it != sources.end(); it++) {
		source = *it;

		if ((source->get_min_hz() <= control_channel_freq) && (source->get_max_hz() >= control_channel_freq)) {
			source_found = true;

			break;
		}
	}

	if (source_found) {
		if (system_type == "smartnet") {
			// what you really need to do is go through all of the sources to find the one with the right frequencies
			smartnet_trunking = make_smartnet_trunking(control_channel_freq, source->get_center(), source->get_rate(),  queue);
			tb->connect(source->get_src_block(),0, smartnet_trunking, 0);
		}

		if (system_type == "p25") {
			// what you really need to do is go through all of the sources to find the one with the right frequencies
			p25_trunking = make_p25_trunking(control_channel_freq, source->get_center(), source->get_rate(),  queue);
			tb->connect(source->get_src_block(),0, p25_trunking, 0);
		}
	}
	return source_found;
}

int main(void)
{
	signal(SIGINT, exit_interupt);
	tb = gr::make_top_block("Smartnet");
	queue = gr::msg_queue::make(100);
	smartnet_parser = new SmartnetParser(); // this has to eventually be generic;
	p25_parser = new P25Parser();

	load_config();

	// Setup the talkgroups from the CSV file
	talkgroups = new Talkgroups();
	//if (talkgroups_file.length() > 0) {
	std::cout<< "Loading Talkgroups..."<<std::endl;
	talkgroups->load_talkgroups(talkgroups_file);
	//}

	if (monitor_system()) {
		tb->start();
		monitor_messages();
		//------------------------------------------------------------------
		//-- stop flow graph execution
		//------------------------------------------------------------------
		std::cout << "stopping flow graph" << std::endl;
		tb->stop();
		tb->wait();
	} else {
		std::cout << "Unable to setup Control Channel Monitor"<< std::endl;
	}




	return 0;

}
