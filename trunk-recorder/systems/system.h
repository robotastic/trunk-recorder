#ifndef SYSTEM_H
#define SYSTEM_H
#include <stdio.h>
#include <boost/log/trivial.hpp>
#include "../talkgroups.h"
//#include "../source.h"
#include "smartnet_trunking.h"
#include "p25_trunking.h"
#include "parser.h"

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
typedef boost::shared_ptr<analog_recorder> analog_recorder_sptr;
class p25_recorder;
typedef boost::shared_ptr<p25_recorder> p25_recorder_sptr;
class p25conventional_recorder;
typedef boost::shared_ptr<p25conventional_recorder> p25conventional_recorder_sptr;



class System
{
        int sys_num;
        unsigned long sys_id;
        unsigned long wacn;
        unsigned long nac;
public:
        enum TalkgroupDisplayFormat { talkGroupDisplayFormat_id=0, talkGroupDisplayFormat_id_tag=1, talkGroupDisplayFormat_tag_id=2};

        Talkgroups *talkgroups;
        p25p2_lfsr *lfsr;
        Source *source;
        std::string talkgroups_file;
        std::string short_name;
        std::string api_key;
        std::string default_mode;
        std::string system_type;
        std::string upload_script;
        int message_count;
        int retune_attempts;
        time_t last_message_time;
        std::string bandplan;
        int bandfreq;
        double bandplan_base;
        double bandplan_high;
        double bandplan_spacing;
        int bandplan_offset;
        double min_call_duration;

        unsigned xor_mask_len;
        const char *xor_mask;
        std::vector<double> control_channels;
        unsigned int current_control_channel;
        std::vector<double> channels;
        std::vector<analog_recorder_sptr> conventional_recorders;
        std::vector<p25conventional_recorder_sptr> conventionalP25_recorders;

        bool qpsk_mod;
        bool audio_archive;
        bool record_unknown;
        bool call_log;
        smartnet_trunking_sptr smartnet_trunking;
        p25_trunking_sptr p25_trunking;

        std::string get_short_name();
        void set_short_name(std::string short_name);
        std::string get_upload_script();
        void set_upload_script(std::string script);
        std::string get_api_key();
        void set_api_key(std::string api_key);
        double get_min_duration();
        void set_min_duration(double duration);
        bool get_audio_archive();
        void set_audio_archive(bool);
        bool get_record_unknown();
        void set_record_unknown(bool);
        bool get_call_log();
        void set_call_log(bool);
        std::string get_system_type();
        unsigned long get_sys_id();
        unsigned long get_wacn();
        unsigned long get_nac();
        void set_xor_mask(unsigned long sys_id,  unsigned long wacn,  unsigned long nac);
        const char * get_xor_mask();
        bool update_status(TrunkMessage message);
        int get_sys_num();
        void set_system_type(std::string);
        std::string get_talkgroups_file();
        Source *get_source();
        void set_source(Source *);
        Talkgroup *find_talkgroup(long tg);
        void set_talkgroups_file(std::string);
        int control_channel_count();
        void add_control_channel(double channel);
        double get_next_control_channel();
        double get_current_control_channel();
        int channel_count();
        void add_channel(double channel);
        void add_conventional_recorder(analog_recorder_sptr rec);
        std::vector<analog_recorder_sptr> get_conventional_recorders();
        void add_conventionalP25_recorder(p25conventional_recorder_sptr rec);
        std::vector<p25conventional_recorder_sptr> get_conventionalP25_recorders();
        std::vector<double> get_channels();
        std::vector<double> get_control_channels();
        System(int sys_id );
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

        bool get_delaycreateoutput();
        void set_delaycreateoutput(bool delaycreateoutput);

        bool get_hideEncrypted();
        void set_hideEncrypted(bool hideEncrypted);

        bool get_hideUnknown();
        void set_hideUnknown(bool hideUnknown);

        boost::property_tree::ptree get_stats();
        boost::property_tree::ptree get_stats_current(float timeDiff);

private:
        TalkgroupDisplayFormat talkgroup_display_format;
        bool d_delaycreateoutput;
        bool d_hideEncrypted;
        bool d_hideUnknown;
};
#endif
