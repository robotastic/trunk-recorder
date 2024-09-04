#ifndef SYSTEM_IMPL_H
#define SYSTEM_IMPL_H
#include "../talkgroups.h"
#include "../unit_tags.h"
#include <boost/foreach.hpp>
#include <boost/log/trivial.hpp>
#include <stdio.h>
//#include "../source.h"
#include "p25_trunking.h"
#include "parser.h"
#include "smartnet_trunking.h"
#include "system.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wint-in-bool-context"
//#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

#include <lfsr/lfsr.h>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <boost/property_tree/ptree.hpp>

class Source;
class analog_recorder;
class p25_recorder;
class dmr_recorder;
class sigmf_recorder;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<analog_recorder> analog_recorder_sptr;
typedef boost::shared_ptr<p25_recorder> p25_recorder_sptr;
typedef boost::shared_ptr<dmr_recorder> dmr_recorder_sptr;
typedef boost::shared_ptr<sigmf_recorder> sigmf_recorder_sptr;
#else
typedef std::shared_ptr<analog_recorder> analog_recorder_sptr;
typedef std::shared_ptr<p25_recorder> p25_recorder_sptr;
typedef std::shared_ptr<dmr_recorder> dmr_recorder_sptr;
typedef std::shared_ptr<sigmf_recorder> sigmf_recorder_sptr;
#endif

class System_impl : public System {
  int sys_num;
  unsigned long sys_id;
  unsigned long wacn;
  unsigned long nac;
  int sys_rfss;
  int sys_site_id;

public:
  Talkgroups *talkgroups;
  UnitTags *unit_tags;
  p25p2_lfsr *lfsr;
  Source *source;
  std::string talkgroups_file;
  std::string channel_file;
  std::string unit_tags_file;
  std::string custom_freq_table_file;
  std::string short_name;
  std::string api_key;
  std::string bcfy_api_key;
  std::string default_mode;
  std::string system_type;
  std::string upload_script;
  int bcfy_system_id;
  int message_count;
  int decode_rate;
  int retune_attempts;
  time_t last_message_time;
  std::string bandplan;
  int bandfreq;
  double bandplan_base;
  double bandplan_high;
  double bandplan_spacing;
  int bandplan_offset;
  int max_dev;
  double filter_width;
  double min_call_duration;
  double max_call_duration;
  double min_transmission_duration;
  bool compress_wav;
  bool conversation_mode;
  bool qpsk_mod;
  double squelch_db;
  double analog_levels;
  double digital_levels;

  unsigned xor_mask_len;
  const char *xor_mask;
  std::vector<double> control_channels;
  unsigned int current_control_channel;
  std::vector<double> channels;
  std::vector<analog_recorder_sptr> conventional_recorders;
  std::vector<p25_recorder_sptr> conventionalP25_recorders;
  std::vector<dmr_recorder_sptr> conventionalDMR_recorders;
  std::vector<sigmf_recorder_sptr> conventionalSIGMF_recorders;
  bool transmission_archive;
  bool audio_archive;
  bool record_unknown;
  bool call_log;

  smartnet_trunking_sptr smartnet_trunking;
  p25_trunking_sptr p25_trunking;

  std::map<unsigned long, std::map<unsigned long, std::time_t>> talkgroup_patches;

  std::string get_short_name();
  void set_short_name(std::string short_name);
  std::string get_upload_script();
  void set_upload_script(std::string script);
  bool get_compress_wav();
  void set_compress_wav(bool compress);
  std::string get_api_key();
  void set_api_key(std::string api_key);
  std::string get_bcfy_api_key();
  void set_bcfy_api_key(std::string bcfy_api_key);
  int get_bcfy_system_id();
  void set_bcfy_system_id(int bcfy_system_id);
  double get_min_duration();
  void set_min_duration(double duration);
  double get_max_duration();
  void set_max_duration(double duration);
  double get_min_tx_duration();
  void set_min_tx_duration(double duration);
  bool get_audio_archive();
  void set_audio_archive(bool);
  bool get_transmission_archive();
  void set_transmission_archive(bool);
  bool get_record_unknown();
  void set_record_unknown(bool);
  bool get_call_log();
  void set_call_log(bool);
  bool get_conversation_mode();
  void set_conversation_mode(bool mode);
  void set_mdc_enabled(bool b);
  void set_fsync_enabled(bool b);
  void set_star_enabled(bool b);
  void set_tps_enabled(bool b);

  bool get_mdc_enabled();
  bool get_fsync_enabled();
  bool get_star_enabled();
  bool get_tps_enabled();

  void set_analog_levels(double r);
  double get_analog_levels();
  void set_digital_levels(double r);
  double get_digital_levels();
  void set_qpsk_mod(bool m);
  bool get_qpsk_mod();
  void set_squelch_db(double s);
  double get_squelch_db();
  void set_max_dev(int max_dev);
  int get_max_dev();
  void set_filter_width(double f);
  double get_filter_width();
  gr::msg_queue::sptr get_msg_queue();
  std::string get_system_type();
  unsigned long get_sys_id();
  unsigned long get_wacn();
  unsigned long get_nac();
  int get_sys_rfss();
  int get_sys_site_id();
  void set_xor_mask(unsigned long sys_id, unsigned long wacn, unsigned long nac);
  const char *get_xor_mask();
  bool update_status(TrunkMessage message);
  bool update_sysid(TrunkMessage message);
  int get_sys_num();
  void set_system_type(std::string);
  std::string get_talkgroups_file();
  std::string get_unit_tags_file();
  Source *get_source();
  void set_source(Source *);
  Talkgroup *find_talkgroup(long tg);
  Talkgroup *find_talkgroup_by_freq(double freq);
  std::string find_unit_tag(long unitID);
  void set_talkgroups_file(std::string);
  void set_channel_file(std::string channel_file);
  bool has_channel_file();
  void set_unit_tags_file(std::string);
  void set_custom_freq_table_file(std::string custom_freq_table_file);
  std::string get_custom_freq_table_file();
  bool has_custom_freq_table_file();
  int control_channel_count();
  int get_message_count();
  void set_message_count(int count);
  int get_decode_rate();
  void set_decode_rate(int rate);
  void add_control_channel(double channel);
  double get_next_control_channel();
  double get_current_control_channel();
  int channel_count();
  void add_channel(double channel);
  void add_conventional_recorder(analog_recorder_sptr rec);
  void add_conventionalP25_recorder(p25_recorder_sptr rec);
  void add_conventionalSIGMF_recorder(sigmf_recorder_sptr rec);
  void add_conventionalDMR_recorder(dmr_recorder_sptr rec);
  std::vector<p25_recorder_sptr> get_conventionalP25_recorders();
  std::vector<analog_recorder_sptr> get_conventional_recorders();
  std::vector<sigmf_recorder_sptr> get_conventionalSIGMF_recorders();
  std::vector<dmr_recorder_sptr> get_conventionalDMR_recorders();
  std::vector<double> get_channels();
  std::vector<double> get_control_channels();
  std::vector<Talkgroup *> get_talkgroups();
  gr::msg_queue::sptr msg_queue;
  System_impl(int sys_id);
  void set_bandplan(std::string);
  std::string get_bandplan();
  void set_bandfreq(int);
  int get_bandfreq();
  void set_bandplan_base(double);
  double get_bandplan_base();
  void set_bandplan_high(double high);
  double get_bandplan_high();
  void set_bandplan_spacing(double);
  double get_bandplan_spacing();
  void set_bandplan_offset(int);
  int get_bandplan_offset();
  void set_talkgroup_display_format(TalkgroupDisplayFormat format);
  TalkgroupDisplayFormat get_talkgroup_display_format();

  bool get_hideEncrypted();
  void set_hideEncrypted(bool hideEncrypted);

  bool get_hideUnknown();
  void set_hideUnknown(bool hideUnknown);

  boost::property_tree::ptree get_stats();
  boost::property_tree::ptree get_stats_current(float timeDiff);

  std::vector<unsigned long> get_talkgroup_patch(unsigned long talkgroup);
  void update_active_talkgroup_patches(PatchData f_data);
  void delete_talkgroup_patch(PatchData f_data);
  void clear_stale_talkgroup_patches();
  void print_active_talkgroup_patches();

  bool get_multiSite();
  void set_multiSite(bool multiSite);

  std::string get_multiSiteSystemName();
  void set_multiSiteSystemName(std::string multiSiteSystemName);

  unsigned long get_multiSiteSystemNumber();
  void set_multiSiteSystemNumber(unsigned long multiSiteSystemNumber);

private:
  TalkgroupDisplayFormat talkgroup_display_format;
  bool d_hideEncrypted;
  bool d_hideUnknown;
  bool d_multiSite;
  std::string d_multiSiteSystemName;
  unsigned long d_multiSiteSystemNumber;

  bool d_mdc_enabled;
  bool d_fsync_enabled;
  bool d_star_enabled;
  bool d_tps_enabled;
};
#endif
