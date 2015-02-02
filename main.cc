#include <tuple>
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
#include "recorder.h"

#ifdef DSD
#include "dsd_recorder.h"
#else
#include "p25_recorder.h"
#endif

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
std::vector<Call *> calls;
Talkgroups *talkgroups;
std::string talkgroups_file;
string system_type;
gr::top_block_sptr tb;
smartnet_trunking_sptr smartnet_trunking;
p25_trunking_sptr p25_trunking;
gr::msg_queue::sptr queue; 

	volatile sig_atomic_t exit_flag = 0;
	int messagesDecodedSinceLastReport = 0;
	float msgs_decoded_per_second = 0;
	time_t lastMsgCountTime = time(NULL);;
 	time_t lastTalkgroupPurge = time(NULL);;
 	SmartnetParser *smartnet_parser;
 	P25Parser *p25_parser;
 	

void exit_interupt(int sig){ // can be called asynchronously
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
                talkgroups_file = node.second.get<std::string>("talkgroupsFile","");
               
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

		system_type = pt.get<std::string>("system.type");
        
    }
    catch (std::exception const& e)
    {
        std::cerr << e.what() << std::endl;
    }
   
}



void start_recorder(long tg_number, double freq) {
	Call * call = new Call(tg_number, freq);
	Talkgroup * talkgroup = talkgroups->find_talkgroup(tg_number);
	bool source_found = false;
	Recorder *recorder;
	call->set_recording(false); // start with the assumption that there are no recorders available.

	for(vector<Source *>::iterator it = sources.begin(); it != sources.end();it++) {
	      Source * source = *it;

		if ((source->get_min_hz() <= freq) && (source->get_max_hz() >= freq)) {
			source_found = true;
			if (talkgroup && (talkgroup->mode == 'A')) {
				recorder = source->get_analog_recorder();
			} else {
			  	recorder = source->get_digital_recorder();

			}
		  	if (recorder) {
		  		std::cout << "Activating recorder" << std::endl;
		  		recorder->activate( tg_number,freq, calls.size());
		  		call->set_recorder(recorder);
		  		call->set_recording(true);
		  	} else {
		  		std::cout << "Not recording call" << std::endl;
		  	}
			
			
		}

	}
	if (!source_found) {
		std::cout << "Recording not started because there was no source covering: " << freq << std::endl;
	}
	calls.push_back(call);
}
void stop_inactive_recorders() {

	char shell_command[200];

 	for(vector<Call *>::iterator it = calls.begin(); it != calls.end();) {
	      Call *call = *it;
	      if ( call->since_last_update()  > 4.0) {
	       
		
			if (call->get_recording() == true) {
				//sprintf(shell_command,"./encode-upload.sh %s > /dev/null 2>&1 &", call->get_recorder()->get_filename());
	        call->get_recorder()->deactivate();
	        //system(shell_command);
	        }
			  
	        
	         it = calls.erase(it);
		  } else {
		    ++it;
		  }//if rx is active
    	}//foreach loggers
}

	void update_recorders(TrunkMessage message) {
		bool call_found = false;

		for(vector<Call *>::iterator it = calls.begin(); it != calls.end(); ++it) {
			Call *call= *it;

			
				if (call->get_talkgroup() == message.talkgroup) {
					if (call->get_freq() != message.freq) {
						std::cout << "Retune - Total calls: " << calls.size() << std::endl;
						// not sure what to do here; looks like we should retune
						call->set_freq(message.freq);
						if (call->get_recording() == true) {
							call->get_recorder()->tune_offset(message.freq);
						}
					}
					call->update();

					call_found = true;
				} else {

					if (call->get_freq() == message.freq) {
						call_found = true;
						std::cout << "Freq in use" << std::endl;
						//different talkgroups on the same freq, that is trouble
					}
				}		
		}


		if (!call_found){
			start_recorder(message.talkgroup, message.freq);
		}
	}

	void handle_message(TrunkMessage message){
		switch(message.message_type) {
			case ASSIGNMENT:
				update_recorders(message);
			break;
			case UPDATE:
				update_recorders(message);
			break;
		}

	}
	void monitor_messages() {
			gr::message::sptr msg;
			time_t currentTime = time(NULL);
			TrunkMessage trunk_message;

	while (1) {
		if(exit_flag){ // my action when signal set it 1
			printf("\n Signal caught!\n");
			return;
		}


	msg = queue->delete_head();
	messagesDecodedSinceLastReport++;
	currentTime = time(NULL);
	   float timeDiff = currentTime - lastMsgCountTime;

	if ((currentTime - lastTalkgroupPurge) >= 1.0 )
	{	
		stop_inactive_recorders();
		lastTalkgroupPurge = currentTime;
	}
	if (system_type == "smartnet") {
		trunk_message = smartnet_parser->parse_message(msg->to_string());
	} 
	if (system_type == "p25") {
		trunk_message = p25_parser->parse_message(msg);
	}
	else {
		std::cout << msg->to_string() << std::endl;
	}
	handle_message(trunk_message);

	if (timeDiff >= 3.0) {
		msgs_decoded_per_second = messagesDecodedSinceLastReport/timeDiff; 
		messagesDecodedSinceLastReport = 0;
		lastMsgCountTime = currentTime;
		if (msgs_decoded_per_second < 30 ) {
			//std::cout << "Control Channel Message Decode Rate: " << msgs_decoded_per_second << "/sec" << std::endl;
		}
		
	}
	msg.reset();
	


	}
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
	if (talkgroups_file.length()) {
		talkgroups->load_talkgroups(talkgroups_file);
	}
          
    if (system_type == "smartnet") {
    	// what you really need to do is go through all of the sources to find the one with the right frequencies
    	smartnet_trunking = make_smartnet_trunking(control_channels[0], sources[0]->get_center(), sources[0]->get_rate(),  queue);
    	tb->connect(sources[0]->get_src_block(),0, smartnet_trunking, 0);
    }

    if (system_type == "p25") {
    	// what you really need to do is go through all of the sources to find the one with the right frequencies
    	p25_trunking = make_p25_trunking(control_channels[0], sources[0]->get_center(), sources[0]->get_rate(),  queue);
    	tb->connect(sources[0]->get_src_block(),0, p25_trunking, 0);
    }

    tb->start();
    monitor_messages();
      //------------------------------------------------------------------
    //-- stop flow graph execution
    //------------------------------------------------------------------
    std::cout << "stopping flow graph" << std::endl;
    tb->stop();
    tb->wait();

    std::cout << "done!" << std::endl;
    return 0;

}