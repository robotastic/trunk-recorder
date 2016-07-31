#include "system.h"

void System::add_control_channel(double channel){
this->control_channels.push_back(channel);
}

std::string System::get_default_mode(){
  return this->default_mode;
}

void System::set_default_mode(std::string def_mode){
   this->default_mode = def_mode;
}

System::System(){

}
bool System::get_qpsk_mod() {
  return this->qpsk_mod;
}
void System::set_qpsk_mod(bool qpsk_mod){
  this->qpsk_mod = qpsk_mod;
}
std::string System::get_system_type(){
  return this->system_type;
}
void System::set_system_type(std::string sys_type){
  this->system_type = sys_type;
}
std::string System::get_talkgroups_file(){
  return this->talkgroups_file;
}
void System::set_talkgroups_file(std::string talkgroups_file){
  this->talkgroups_file = talkgroups_file;
}
double System::get_current_control_channel() {
  return this->control_channels[0];
}
