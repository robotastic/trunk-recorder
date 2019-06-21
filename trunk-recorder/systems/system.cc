#include "system.h"


std::string System::get_api_key() {
  return this->api_key;
}

void System::set_api_key(std::string api_key) {
  this->api_key = api_key;
}

std::string System::get_short_name() {
  return this->short_name;
}

void System::set_short_name(std::string short_name) {
  this->short_name = short_name;
}

std::string System::get_upload_script() {
  return this->upload_script;
}

void System::set_upload_script(std::string script) {
  this->upload_script = script;
}

double System::get_min_duration() {
  return this->min_call_duration;
}

void System::set_min_duration(double duration) {
  this->min_call_duration = duration;
}


System::System(int sys_num) {
  this->sys_num = sys_num;
  sys_id = 0;
  wacn = 0;
  nac = 0;
  current_control_channel = 0;
  xor_mask_len=0;
  xor_mask = NULL;
  // Setup the talkgroups from the CSV file
  talkgroups = new Talkgroups();
  d_delaycreateoutput = false;
  d_hideEncrypted = false;
  d_hideUnknown = false;
  retune_attempts = 0;
  message_count = 0;
}

void System::set_xor_mask(unsigned long sys_id,  unsigned long wacn,  unsigned long nac){
  if(sys_id && wacn && nac) {
    this->sys_id = sys_id;
    this->wacn = wacn;
    this->nac = nac;
    BOOST_LOG_TRIVIAL(info) << "Setting XOR Mask: System ID " << std::dec << sys_id << " WACN: " << wacn << " NAC: " << nac <<  std::dec;
    if(sys_id && wacn && nac) {
      lfsr = new p25p2_lfsr(nac, sys_id, wacn);
      xor_mask =  lfsr->getXorChars(xor_mask_len);
      /*
      BOOST_LOG_TRIVIAL(info) << "XOR Mask len: " << xor_mask_len;
      for (unsigned i=0; i<xor_mask_len; i++) {
        std::cout << (short)xor_mask[i] << ", ";
      }*/
    }
  }

}
bool System::update_status(TrunkMessage message) {
 if(!sys_id || !wacn || !nac) {
   sys_id = message.sys_id;
   wacn = message.wacn;
   nac = message.nac;
   BOOST_LOG_TRIVIAL(info) << "[" << short_name << "]\tDecoding System ID "
	   << std::hex << std::uppercase << message.sys_id << " WACN: "
	   << std::hex << std::uppercase << message.wacn << " NAC: " << std::hex << std::uppercase << message.nac;
   if(sys_id && wacn && nac) {
     lfsr = new p25p2_lfsr(nac, sys_id, wacn);
     xor_mask =  lfsr->getXorChars(xor_mask_len);
     /*
     BOOST_LOG_TRIVIAL(info) << "XOR Mask len: " << xor_mask_len;
     for (unsigned i=0; i<xor_mask_len; i++) {
       std::cout << (short)xor_mask[i] << ", ";
     }*/
   }
  return true;
 }
 return false;
}

const char * System::get_xor_mask(){
  return xor_mask;
}

int System::get_sys_num() {
  return this->sys_num;
}


unsigned long System::get_sys_id() {
  return this->sys_id;
}

unsigned long System::get_nac() {
  return this->nac;
}

unsigned long System::get_wacn() {
  return this->wacn;
}

bool System::get_call_log() {
  return this->call_log;
}

void System::set_call_log(bool call_log) {
  this->call_log = call_log;
}

bool System::get_audio_archive() {
  return this->audio_archive;
}

void System::set_audio_archive(bool audio_archive) {
  this->audio_archive = audio_archive;
}


bool System::get_record_unknown() {
  return this->record_unknown;
}

void System::set_record_unknown(bool unknown) {
  this->record_unknown = unknown;
}

std::string System::get_system_type() {
  return this->system_type;
}

void System::set_system_type(std::string sys_type) {
  this->system_type = sys_type;
}

std::string System::get_talkgroups_file() {
  return this->talkgroups_file;
}

void System::set_talkgroups_file(std::string talkgroups_file) {
  BOOST_LOG_TRIVIAL(info) << "Loading Talkgroups...";
  this->talkgroups_file = talkgroups_file;
  this->talkgroups->load_talkgroups(talkgroups_file);
}

Source *System::get_source(){
  return this->source;
}

void System::set_source(Source *s) {
  this->source = s;
}

Talkgroup * System::find_talkgroup(long tg_number) {
  return talkgroups->find_talkgroup(tg_number);
}

std::vector<double> System::get_channels(){
  return channels;
}


int System::channel_count() {
  return channels.size();
}

void System::add_conventional_recorder(analog_recorder_sptr rec) {
    conventional_recorders.push_back(rec);
  }
std::vector<analog_recorder_sptr> System::get_conventional_recorders() {
  return conventional_recorders;
}

void System::add_conventionalP25_recorder(p25conventional_recorder_sptr rec) {
	conventionalP25_recorders.push_back(rec);
}

std::vector<p25conventional_recorder_sptr> System::get_conventionalP25_recorders() {
  return conventionalP25_recorders;
}

void System::add_channel(double channel) {
  if (channels.size() == 0) {
    channels.push_back(channel);
  } else {
    if (std::find(channels.begin(), channels.end(), channel) == channels.end()) {
      channels.push_back(channel);
    }
  }
}


int System::control_channel_count() {
  return control_channels.size();
}

std::vector<double> System::get_control_channels() {
  return control_channels;
}

void System::add_control_channel(double control_channel) {
  if (control_channels.size() == 0) {
    control_channels.push_back(control_channel);
  } else {
    if (std::find(control_channels.begin(), control_channels.end(),
                  control_channel) == control_channels.end()) {
      control_channels.push_back(control_channel);
    }
  }
}

double System::get_current_control_channel() {
  return this->control_channels[current_control_channel];
}

double System::get_next_control_channel() {
  current_control_channel++;
  if (current_control_channel >= control_channels.size()) {
    current_control_channel = 0;
  }
  return this->control_channels[current_control_channel];
}

void System::set_bandplan(std::string bandplan) {
  this->bandplan = bandplan;
}

std::string System::get_bandplan() {
  return this->bandplan;
}

void System::set_bandfreq(int freq) {
  this->bandfreq = freq;
}

int System::get_bandfreq() {
  return this->bandfreq;
}

void System::set_bandplan_high(double high) {
  this->bandplan_high = high;
}

double System::get_bandplan_high() {
  return this->bandplan_high / 1000000;
}

void System::set_bandplan_base(double base) {
  this->bandplan_base = base;
}

double System::get_bandplan_base() {
  return this->bandplan_base / 1000000;
}

void System::set_bandplan_spacing(double space) {
  this->bandplan_spacing = space / 1000000;
}

double System::get_bandplan_spacing() {
  return this->bandplan_spacing;
}

void System::set_bandplan_offset(int offset) {
  this->bandplan_offset = offset;
}

int System::get_bandplan_offset() {
  return this->bandplan_offset;
}

void System::set_talkgroup_display_format(TalkgroupDisplayFormat format){
  talkgroup_display_format = format;
}

System::TalkgroupDisplayFormat System::get_talkgroup_display_format(){
  return talkgroup_display_format;
}

bool System::get_delaycreateoutput(){
  return d_delaycreateoutput;
}

void System::set_delaycreateoutput(bool delaycreateoutput){
  d_delaycreateoutput = delaycreateoutput;
}

bool System::get_hideEncrypted(){
  return d_hideEncrypted;
}
void System::set_hideEncrypted(bool hideEncrypted){
  d_hideEncrypted = hideEncrypted;
}

bool System::get_hideUnknown(){
  return d_hideUnknown;
}

void System::set_hideUnknown(bool hideUnknown){
  d_hideUnknown = hideUnknown;
}

boost::property_tree::ptree System::get_stats()
{
  boost::property_tree::ptree system_node;
  system_node.put("id",           this->get_sys_num());
  system_node.put("name",         this->get_short_name());
  system_node.put("type",         this->get_system_type());
  system_node.put("sysid",        this->get_sys_id());
  system_node.put("wacn",         this->get_wacn());
  system_node.put("nac",          this->get_nac());

  return system_node;
}

boost::property_tree::ptree System::get_stats_current(float timeDiff)
{
  boost::property_tree::ptree system_node;
  system_node.put("id",           this->get_sys_num());
  system_node.put("decoderate",   this->message_count / timeDiff);

  return system_node;
}
