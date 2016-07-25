#ifndef SYSTEM_H
#define SYSTEM_H
#include <stdio.h>
#include "smartnet_trunking.h"
#include "p25_trunking.h"

class System
{
public:
  std::string talkgroups_file;
  std::string default_mode;
  std::string system_type;

std::vector<double> control_channels;
int current_control_channel = 0;
bool qpsk_mod = true;
smartnet_trunking_sptr smartnet_trunking;
p25_trunking_sptr p25_trunking;
std::string get_default_mode();
void set_default_mode(std::string def_mode);
bool get_qpsk_mod();
void set_qpsk_mod(bool);
std::string get_system_type();
void set_system_type(std::string);
std::string get_talkgroups_file();
void set_talkgroups_file(std::string);
void add_control_channel(double channel);
double get_current_control_channel();
System( );
};
#endif
