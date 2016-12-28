#ifndef SYSTEM_H
#define SYSTEM_H
#include <stdio.h>
#include "smartnet_trunking.h"
#include "p25_trunking.h"

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

        std::vector<double> control_channels;
        int current_control_channel;
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

        System(int sys_id );
};
#endif
