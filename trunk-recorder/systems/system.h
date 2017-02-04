#ifndef SYSTEM_H
#define SYSTEM_H
#include <stdio.h>

#include "smartnet_trunking.h"
#include "p25_trunking.h"

class analog_recorder;
typedef boost::shared_ptr<analog_recorder> analog_recorder_sptr;
class p25_recorder;
typedef boost::shared_ptr<p25_recorder> p25_recorder_sptr;

class System
{
        int sys_id;
public:
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

        std::vector<double> control_channels;
        int current_control_channel;
        std::vector<double> channels;
        std::vector<analog_recorder_sptr> conventional_recorders;
        std::vector<p25_recorder_sptr> conventionalP25_recorders;

        bool qpsk_mod;
        smartnet_trunking_sptr smartnet_trunking;
        p25_trunking_sptr p25_trunking;
        std::string get_default_mode();
        void set_default_mode(std::string def_mode);
        std::string get_short_name();
        void set_short_name(std::string short_name);
        std::string get_upload_script();
        void set_upload_script(std::string script);
        std::string get_api_key();
        void set_api_key(std::string api_key);
        bool get_qpsk_mod();
        void set_qpsk_mod(bool);
        std::string get_system_type();
        int get_sys_id();
        void set_system_type(std::string);
        std::string get_talkgroups_file();
        void set_talkgroups_file(std::string);
        int control_channel_count();
        void add_control_channel(double channel);
        double get_next_control_channel();
        double get_current_control_channel();
        int channel_count();
        void add_channel(double channel);
        void add_conventional_recorder(analog_recorder_sptr rec);
        std::vector<analog_recorder_sptr> get_conventional_recorders();
        void add_conventionalP25_recorder(p25_recorder_sptr rec);
        std::vector<p25_recorder_sptr> get_conventionalP25_recorders();
        std::vector<double> get_channels();
        System(int sys_id );
        void set_bandplan(std::string);
        std::string get_bandplan();
};
#endif
