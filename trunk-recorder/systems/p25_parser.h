#ifndef P25_PARSE_H
#define P25_PARSE_H
#include "parser.h"
#include <bitset>
#include <boost/dynamic_bitset.hpp>
#include <boost/log/trivial.hpp>
#include <gnuradio/message.h>
#include "system.h"
#include "system_impl.h"
#include <iomanip>
#include <iostream>
#include <map>
#include <vector>

#include "../csv_helper.h"
#include <csv-parser/csv.hpp>

struct Freq_Table {
  unsigned long id;
  long offset;
  unsigned long step;
  unsigned long frequency;
  bool phase2_tdma;
  int slots_per_carrier;
  double bandwidth;
};

class P25Parser : public TrunkParser {
  std::map<int, std::map<int, Freq_Table>> freq_tables;
  std::map<int, Freq_Table>::iterator it;
  bool custom_freq_table_loaded = false;

public:
  P25Parser();
  long get_tdma_slot(int chan_id, int sys_num);
  double get_bandwidth(int chan_id, int sys_num);
  std::vector<TrunkMessage> decode_mbt_data(unsigned long opcode, boost::dynamic_bitset<> &header, boost::dynamic_bitset<> &mbt_data, unsigned long link_id, unsigned long nac, int sys_num);
  std::vector<TrunkMessage> decode_tsbk(boost::dynamic_bitset<> &tsbk, unsigned long nac, int sys_num);
  unsigned long bitset_shift_mask(boost::dynamic_bitset<> &tsbk, int shift, unsigned long long mask);
  unsigned long bitset_shift_left_mask(boost::dynamic_bitset<> &tsbk, int shift, unsigned long long mask);
  std::string channel_id_to_freq_string(int chan_id, int sys_num);
  void print_bitset(boost::dynamic_bitset<> &tsbk);
  void add_freq_table(int freq_table_id, Freq_Table table, int sys_num);
  void load_freq_table(std::string custom_freq_table_file, int sys_num);
  double channel_id_to_frequency(int chan_id, int sys_num);
  std::string channel_to_string(int chan, int sys_num);
  std::vector<TrunkMessage> parse_message(gr::message::sptr msg, System *system);
};

#endif