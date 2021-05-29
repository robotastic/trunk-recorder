#ifndef OPENMHZ_UPLOADER_H
#define OPENMHZ_UPLOADER_H



class Uploader;
#include <time.h>
#include <vector>
#include <curl/curl.h>

#include <gr_blocks/decoder_wrapper.h>
#include "../../call_concluder/call_concluder.h"
#include "../plugin-common.h"


class Openmhz_Uploader {
public:
  static int upload(Call_Data_t call_info);
  static int call_end(plugin_t * const plugin, Call_Data_t call_info);
  static int init(plugin_t * const plugin, Config* config);
  static int parse_config(plugin_t * const plugin, boost::property_tree::ptree::value_type &cfg); 
protected:
  static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
};





struct Openmhz_System_Key {
  std::string api_key;
  std::string short_name;
};


struct Openmhz_Uploader_Data {
  std::vector<Openmhz_System_Key> keys;
  std::string openmhz_server;
};

#endif
