#include "openmhz_uploader.h"
#include "../plugin-common.h"

size_t OpenmhzUploader::write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}
int OpenmhzUploader::upload(Call_Data call_info) {
  std::ostringstream freq;
  std::string freq_string;
  freq << std::fixed << std::setprecision(0);
  freq << call_info.freq;

  std::ostringstream call_length;
  std::string call_length_string;
  call_length << std::fixed << std::setprecision(0);
  call_length << call_info.length;

  std::ostringstream source_list;
  std::string source_list_string;
  source_list << std::fixed << std::setprecision(2);
  source_list << "[";

  if (call_info.source_count != 0) {
    std::vector<Call_Source> call_source_list = call_info.source_list;

    for (int i = 0; i < call_info.source_count; i++) {
      source_list << "{ \"pos\": " << std::setprecision(2) << call_source_list[i].position << ", \"src\": " << std::setprecision(0) << call_source_list[i].source << " }";

      if (i < (call_info.source_count - 1)) {
        source_list << ", ";
      } else {
        source_list << "]";
      }
    }
  } else {
    source_list << "]";
  }

  std::ostringstream freq_list;
  std::string freq_list_string;
  freq_list << std::fixed << std::setprecision(2);
  freq_list << "[";

  if (call_info.freq_count != 0) {
    Call_Freq *call_freq_list = call_info.freq_list;

    for (int i = 0; i < call_info.freq_count; i++) {
      freq_list << "{ \"pos\": " << std::setprecision(2) << call_freq_list[i].position << ", \"freq\": " << std::setprecision(0) << call_info.freq_list[i].freq << ", \"len\": " << call_info.freq_list[i].total_len << ", \"errors\": " << call_freq_list[i].error_count << ", \"spikes\": " << call_freq_list[i].spike_count << " }";

      if (i < (call_info.freq_count - 1)) {
        freq_list << ", ";
      } else {
        freq_list << "]";
      }
    }
  } else {
    freq_list << "]";
  }

  CURL *curl;
  CURLMcode res;
  CURLM *multi_handle;
  int still_running = 0;
  std::string response_buffer;
  freq_string = freq.str();
  freq_list_string = freq_list.str();
  source_list_string = source_list.str();
  call_length_string = call_length.str();

  struct curl_httppost *formpost = NULL;
  struct curl_httppost *lastptr = NULL;
  struct curl_slist *headerlist = NULL;

  /* Fill in the file upload field. This makes libcurl load data from
     the given file name when curl_easy_perform() is called. */
  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "call",
               CURLFORM_FILE, call_info.converted,
               CURLFORM_CONTENTTYPE, "application/octet-stream",
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "freq",
               CURLFORM_COPYCONTENTS, freq_string.c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "start_time",
               CURLFORM_COPYCONTENTS, boost::lexical_cast<std::string>(call_info.start_time).c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "stop_time",
               CURLFORM_COPYCONTENTS, boost::lexical_cast<std::string>(call_info.stop_time).c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "call_length",
               CURLFORM_COPYCONTENTS, call_length_string.c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "talkgroup_num",
               CURLFORM_COPYCONTENTS, boost::lexical_cast<std::string>(call_info.talkgroup).c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "emergency",
               CURLFORM_COPYCONTENTS, boost::lexical_cast<std::string>(call_info.emergency).c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "api_key",
               CURLFORM_COPYCONTENTS, call_info.api_key.c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "source_list",
               CURLFORM_COPYCONTENTS, source_list_string.c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "freq_list",
               CURLFORM_COPYCONTENTS, freq_list_string.c_str(),
               CURLFORM_END);

  curl = curl_easy_init();
  multi_handle = curl_multi_init();

  /* initialize custom header list (stating that Expect: 100-continue is not wanted */
  headerlist = curl_slist_append(headerlist, "Expect:");
  if (curl && multi_handle) {
    std::string url = call_info.upload_server + "/" + call_info.short_name + "/upload";

    /* what URL that receives this POST */
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "TrunkRecorder1.0");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);

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

    res = curl_multi_cleanup(multi_handle);

    /* always cleanup */
    curl_easy_cleanup(curl);

    /* then cleanup the formpost chain */
    curl_formfree(formpost);

    /* free slist */
    curl_slist_free_all(headerlist);

    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    if (res == CURLM_OK && response_code == 200) {
      struct stat file_info;
      stat(call_info.converted, &file_info);

      BOOST_LOG_TRIVIAL(info) << "[" << call_info.short_name << "]\tTG: " << call_info.talkgroup << "\tFreq: " << FormatFreq(call_info.freq) << "\tOpenMHz Upload Success - file size: " << file_info.st_size;
      ;
      return 0;
    }
  }
  BOOST_LOG_TRIVIAL(error) << "[" << call_info.short_name << "]\tTG: " << call_info.talkgroup << "\tFreq: " << FormatFreq(call_info.freq) << "\tOpenMHz Upload Error: " << response_buffer;
  return 1;
}

int OpenmhzUploader::call_end(plugin_t * const plugin, Call_Data call_info){
    return upload(call_info);

}

int OpenmhzUploader::init(plugin_t * const plugin, Config* config) {
/*    stat_plugin_t *stat_data = (stat_plugin_t*)plugin->plugin_data;

    stat_data->config = config;

    stats.initialize(config, &socket_connected, plugin);
    return 0;*/
    return 0;
}


MODULE_EXPORT plugin_t *openmhz_uploader_plugin_new() {
    //stat_data->config = NULL;
    //stat_data->sources = NULL;
    //stat_data->systems = NULL;

    plugin_t *plug_data = (plugin_t *)malloc(sizeof(plugin_t));

    plug_data->init = OpenmhzUploader::init;
    plug_data->parse_config = NULL;
    plug_data->start = NULL;
    plug_data->stop = NULL;
    plug_data->poll_one = NULL;
    plug_data->signal = NULL;
    plug_data->audio_stream = NULL;
    plug_data->call_start = NULL;
    plug_data->call_end = OpenmhzUploader::call_end;
    plug_data->calls_active = NULL;
    plug_data->setup_recorder = NULL;
    plug_data->setup_system = NULL;
    plug_data->setup_systems = NULL;
    plug_data->setup_sources = NULL;
    plug_data->setup_config = NULL;
    plug_data->system_rates = NULL;

    //plug_data->plugin_data = stat_data;

    return plug_data;
}