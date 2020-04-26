#ifndef UPLOADER_H
#define UPLOADER_H

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <boost/log/trivial.hpp>
#include <curl/curl.h>

#include "../formatter.h"
#include "../call.h"

struct call_data_t {
    long talkgroup;
    double freq;
    long start_time;
    long stop_time;
    bool encrypted;
    bool emergency;
    bool audio_archive;
    char filename[255];
    char status_filename[255];
    char converted[255];
    char file_path[255];
    std::string upload_server;
    std::string bcfy_api_key;
    std::string bcfy_calls_server;
    std::string api_key;
    std::string short_name;
    int bcfy_system_id;
    int tdma_slot;
    int length;
    bool phase2_tdma;
    long source_count;
    std::vector<Call_Source> source_list;
    long freq_count;
    Call_Freq freq_list[50];
    long error_list_count;
    Call_Error error_list[50];
};


class Uploader {
public:
    virtual int upload(struct call_data_t *call) = 0;
protected:
    static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
};

#endif
