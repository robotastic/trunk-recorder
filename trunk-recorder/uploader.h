#ifndef UPLOADER_H
#define UPLOADER_H

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/detail/socket_option.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/regex.hpp>
#include <boost/log/trivial.hpp>
#include <pthread.h>
#include <sstream>

#include "config.h"
#include "systems/system.h"

class Call;

#include "call.h"

struct call_data_t {
        long thread_id;
        long talkgroup;
        double freq;
        long start_time;

        long stop_time;
        bool encrypted;
        bool emergency;
        char filename[160];
        char converted[160];
        std::string upload_server;
        std::string server;
        std::string scheme;
        std::string hostname;
        std::string port;
        std::string path;
        std::string api_key;
        std::string short_name;
        int tdma;
        long source_count;
        Call_Source source_list[50];
        long freq_count;
        Call_Freq freq_list[50];
};

struct server_data_t {
        std::string upload_server;
        std::string server;
        std::string scheme;
        std::string hostname;
        std::string port;
        std::string path;
};

int upload(struct call_data_t *call);
void *convert_upload_call(void *thread_arg);
void send_call(Call *call, System *sys, Config config);


#endif
