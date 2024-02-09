#include <curl/curl.h>
#include <iomanip>
#include <time.h>
#include <vector>
#include <fstream>
#include <iostream>

#include "../../trunk-recorder/call_concluder/call_concluder.h"
#include "../../trunk-recorder/plugin_manager/plugin_api.h"
#include "../../trunk-recorder/plugin_manager/plugin_api.h"
#include "../../lib/json.hpp"
#include "../../lib/base64.h"
#include <boost/dll/alias.hpp>
#include <boost/foreach.hpp>
#include <sys/stat.h>

using json = nlohmann::json;

struct TPNG_Uploader_Data {
  std::string token;
  std::string tpng_server;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
};

class TPNG_Uploader : public Plugin_Api {
  TPNG_Uploader_Data data;

public:
  static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
  }

  std::stringstream create_call_json(Call_Data_t call_info) {
    // Create the JSON -- Moderatly borrowed
    std::stringstream json;

    json << "{\n";
    json << "\"freq\": " << std::fixed << std::setprecision(0) << call_info.freq << ",\n";
    json << "\"start_time\": " << call_info.start_time << ",\n";
    json << "\"stop_time\": " << call_info.stop_time << ",\n";
    json << "\"emergency\": " << call_info.emergency << ",\n";
    json << "\"priority\": " << call_info.priority << ",\n";
    json << "\"mode\": " << call_info.mode << ",\n";
    json << "\"duplex\": " << call_info.duplex << ",\n";
    json << "\"encrypted\": " << call_info.encrypted << ",\n";
    json << "\"call_length\": " << call_info.length << ",\n";
    json << "\"talkgroup\": " << call_info.talkgroup << ",\n";
    json << "\"talkgroup_tag\": \"" << call_info.talkgroup_alpha_tag << "\",\n";
    json << "\"talkgroup_description\": \"" << call_info.talkgroup_description << "\",\n";
    json << "\"talkgroup_group_tag\": \"" << call_info.talkgroup_tag << "\",\n";
    json << "\"talkgroup_group\": \"" << call_info.talkgroup_group << "\",\n";
    json << "\"audio_type\": \"" << call_info.audio_type << "\",\n";
    json << "\"short_name\": \"" << call_info.short_name << "\",\n";

    if (call_info.patched_talkgroups.size() > 1) {
      json << "\"patched_talkgroups\": [";
      bool first = true;
      BOOST_FOREACH (auto &TGID, call_info.patched_talkgroups) {
        if (!first) {
          json << ",";
        }
        first = false;
        json << (int)TGID;
      }
      json << "],\n";
    }

    json << "\"freqList\": [ ";
    for (std::size_t i = 0; i < call_info.transmission_error_list.size(); i++) {
      if (i != 0) {
        json << ", ";
      }
      json << "{\"freq\": " << std::fixed << std::setprecision(0) << call_info.freq << ", \"time\": " << call_info.transmission_error_list[i].time << ", \"pos\": " << std::fixed << std::setprecision(2) << call_info.transmission_error_list[i].position << ", \"len\": " << call_info.transmission_error_list[i].total_len << ", \"error_count\": \"" << std::fixed << std::setprecision(0) << call_info.transmission_error_list[i].error_count << "\", \"spike_count\": \"" << call_info.transmission_error_list[i].spike_count << "\"}";
    }
    json << " ],\n";
    json << "\"srcList\": [ ";

    for (std::size_t i = 0; i < call_info.transmission_source_list.size(); i++) {
      if (i != 0) {
        json << ", ";
      }
      json << "{\"src\": " << std::fixed << call_info.transmission_source_list[i].source << ", \"time\": " << call_info.transmission_source_list[i].time << ", \"pos\": " << std::fixed << std::setprecision(2) << call_info.transmission_source_list[i].position << ", \"emergency\": " << call_info.transmission_source_list[i].emergency << ", \"signal_system\": \"" << call_info.transmission_source_list[i].signal_system << "\", \"tag\": \"" << call_info.transmission_source_list[i].tag << "\"}";
    }
    json << " ]\n";
    json << "}\n";

    return json;
  }

  std::string base64_encode_m4a(const std::string& path) {
    std::vector<char> temp;

    std::ifstream infile;
    infile.open(path, std::ios::binary);     // Open file in binary mode
    if (infile.is_open()) {
        while (!infile.eof()) {
            char c = (char)infile.get();
            temp.push_back(c);
        }
        infile.close();
    }
    else return "File could not be opened";
    std::string ret(temp.begin(), temp.end() - 1);
    ret = macaron::Base64::Encode(ret);

    return ret;
}

  int upload(Call_Data_t call_info) {

    std::string token = this->data.token;
    std::stringstream json_buffer = create_call_json(call_info);
    nlohmann::json call_json = nlohmann::json::parse(json_buffer.str());
    std::string base64_audio = base64_encode_m4a(call_info.converted);

    if (token.size() == 0) {
      return 0;
    }

    nlohmann::json payload = {
      {"recorder", token},
      {"name", call_info.filename},
      {"json", call_json},
      {"audio_file", base64_audio}
    };
    std::string post_data = payload.dump();
   
    CURLM *multi_handle;
    CURL *curl;
    curl = curl_easy_init();
    multi_handle = curl_multi_init();
    std::string response_buffer;

    if (curl && multi_handle) {
      std::string url = data.tpng_server + "/radio/transmission/create";

      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());

      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);

      int still_running = 0;
      curl_multi_add_handle(multi_handle, curl);
      curl_multi_perform(multi_handle, &still_running);

      while (still_running) {
        struct timeval timeout;
        int rc;       /* select() return code */
        CURLMcode mc; /* curl_multi_fdset() return code */

        fd_set fdread;
        fd_set fdwrite;
        fd_set fdexcep;
        int maxfd = -1;

        long curl_timeo = -1;

        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);

        /* set a suitable timeout to play around with */
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        curl_multi_timeout(multi_handle, &curl_timeo);
        if (curl_timeo >= 0) {
          timeout.tv_sec = curl_timeo / 1000;
          if (timeout.tv_sec > 1)
            timeout.tv_sec = 1;
          else
            timeout.tv_usec = (curl_timeo % 1000) * 1000;
        }

        /* get file descriptors from the transfers */
        mc = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

        if (mc != CURLM_OK) {
          fprintf(stderr, "curl_multi_fdset() failed, code %d.\n", mc);
          break;
        }

        /* On success the value of maxfd is guaranteed to be >= -1. We call
          select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
          no fds ready yet so we call select(0, ...) --or Sleep() on Windows--
          to sleep 100ms, which is the minimum suggested value in the
          curl_multi_fdset() doc. */

        if (maxfd == -1) {
          /* Portable sleep for platforms other than Windows. */
          struct timeval wait = {0, 100 * 1000}; /* 100ms */
          rc = select(0, NULL, NULL, NULL, &wait);
        } else {
          /* Note that on some platforms 'timeout' may be modified by select().
            If you need access to the original value save a copy beforehand. */
          rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
        }

        switch (rc) {
        case -1:
          /* select error */
          break;
        case 0:
        default:
          /* timeout or readable/writable sockets */
          curl_multi_perform(multi_handle, &still_running);
          break;
        }
      }

      long response_code;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

      CURLMcode res = curl_multi_cleanup(multi_handle);

      /* always cleanup */
      curl_easy_cleanup(curl);

      if (res == CURLM_OK) {
        struct stat file_info;
        stat(call_info.converted, &file_info);

        BOOST_LOG_TRIVIAL(info) << "[" << call_info.short_name << "]\t\033[0;34m" << call_info.call_num << "C\033[0m\tTG: " << call_info.talkgroup_display << "\tFreq: " << format_freq(call_info.freq) << "\tTrunk-PlayerNG Upload Success - file size: " << file_info.st_size;
        ;
        return 0;
      }
    }
    BOOST_LOG_TRIVIAL(error) << "[" << call_info.short_name << "]\t\033[0;34m" << call_info.call_num << "C\033[0m\tTG: " << call_info.talkgroup_display << "\tFreq: " << format_freq(call_info.freq) << "\tTrunk-PlayerNG Upload Error: " << response_buffer;
    return 1;
  }

  int call_end(Call_Data_t call_info) {
    return upload(call_info);
  }

  int parse_config(json config_data) {

    // Tests to see if the url value exists in the config file
    bool upload_server_exists = config_data.contains("url");
    if (!upload_server_exists) {
      return 1;
    }

    this->data.tpng_server = config_data.value("url", "");
    this->data.token = config_data.value("token", "");
    BOOST_LOG_TRIVIAL(info) << "Trunk-PlayerNG Server: " << this->data.tpng_server;

    boost::regex ex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
    boost::cmatch what;

    if (!regex_match(this->data.tpng_server.c_str(), what, ex)) {
      BOOST_LOG_TRIVIAL(error) << "Unable to parse Server URL\n";
      return 1;
    }

    if (this->data.token == "") {
      BOOST_LOG_TRIVIAL(error) << "Trunk-PlayerNG Server set, but no token is setted\n";
      return 1;
    }
    return 0;
  }

  static boost::shared_ptr<TPNG_Uploader> create() {
    return boost::shared_ptr<TPNG_Uploader>(
        new TPNG_Uploader());
  }
};

BOOST_DLL_ALIAS(
    TPNG_Uploader::create,
    create_plugin
)
