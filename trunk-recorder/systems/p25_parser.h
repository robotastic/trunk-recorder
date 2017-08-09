#ifndef P25_PARSE_H
#define P25_PARSE_H
#include "parser.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include <boost/dynamic_bitset.hpp>
#include <boost/log/trivial.hpp>
#include <gnuradio/message.h>
#include <bitset>

struct Channel {
								unsigned long id;
								unsigned long offset;
								unsigned long step;
								unsigned long frequency;
								bool phase2_tdma;
								int slots_per_carrier;
								double bandwidth;
};

class P25Parser : public TrunkParser
{
								std::map<int,Channel> channels;
								std::map<int,Channel>::iterator it;
public:
								P25Parser();
								long get_tdma_slot(int chan_id);
								double get_bandwidth(int chan_id);
								std::vector<TrunkMessage> decode_mbt_data(unsigned long opcode, boost::dynamic_bitset<>& header, boost::dynamic_bitset<>& mbt_data,  unsigned long link_id,  unsigned long nac, int sys_num); 
								std::vector<TrunkMessage> decode_tsbk(boost::dynamic_bitset<> &tsbk, unsigned long nac, int sys_num);
								unsigned long bitset_shift_mask(boost::dynamic_bitset<> &tsbk, int shift, unsigned long long mask);
								std::string  channel_id_to_string(int chan_id);
								void print_bitset(boost::dynamic_bitset<> &tsbk);
								void add_channel(int chan_id, Channel chan);
								double channel_id_to_frequency(int chan_id);
								std::vector<TrunkMessage> parse_message(gr::message::sptr msg);
};

#endif
