#ifndef OPENMHZ_UPLOADER_H
#define OPENMHZ_UPLOADER_H



class Uploader;
#include <time.h>
#include <vector>
#include <curl/curl.h>

#include <gr_blocks/decoder_wrapper.h>
#include "../../call_concluder/call_concluder.h"
#include "../plugin-common.h"


class OpenmhzUploader {
public:
  static int upload(Call_Data call_info);
  static int call_end(plugin_t * const plugin, Call_Data call_info);
  static int init(plugin_t * const plugin, Config* config);
protected:
  static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
};






typedef struct stat_plugin_t stat_plugin_t;

struct stat_plugin_t {
  std::vector<Source *> sources;
  std::vector<System *> systems;
  std::vector<Call *> calls;
  Config* config;
};

#endif
