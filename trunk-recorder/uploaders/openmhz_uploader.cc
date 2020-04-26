#include "uploader.h"
#include "openmhz_uploader.h"

int OpenmhzUploader::upload(struct call_data_t *call) {
  std::ostringstream freq;
  freq << std::fixed << std::setprecision(0);
  freq << call->freq;

  std::ostringstream call_length;
  call_length << std::fixed << std::setprecision(0);
  call_length << call->length;

  std::ostringstream source_list;
  source_list << std::fixed << std::setprecision(2);
  source_list << "[";

  if (call->source_count != 0) {
    std::vector<Call_Source> call_source_list = call->source_list;

    for (int i = 0; i < call->source_count; i++) {
      source_list << "{ \"pos\": " << std::setprecision(2) << call_source_list[i].position << ", \"src\": " << std::setprecision(0) << call_source_list[i].source << " }";

      if (i < (call->source_count - 1)) {
        source_list <<  ", ";
      } else {
        source_list << "]";
      }
    }
  } else {
    source_list << "]";
  }

  std::ostringstream freq_list;
  freq_list << std::fixed << std::setprecision(2);
  freq_list << "[";

  if (call->freq_count != 0) {
    Call_Freq *call_freq_list = call->freq_list;

    for (int i = 0; i < call->freq_count; i++) {
      freq_list << "{ \"pos\": " << std::setprecision(2) << call_freq_list[i].position << ", \"freq\": " << std::setprecision(0) << call->freq_list[i].freq << ", \"len\": " << call->freq_list[i].total_len << ", \"errors\": " << call_freq_list[i].error_count << ", \"spikes\": " << call_freq_list[i].spike_count << " }";

      if (i < (call->freq_count - 1)) {
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

  struct curl_httppost *formpost = NULL;
  struct curl_httppost *lastptr = NULL;
  struct curl_slist *headerlist = NULL;

  /* Fill in the file upload field. This makes libcurl load data from
     the given file name when curl_easy_perform() is called. */
  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "call",
               CURLFORM_FILE, call->converted,
               CURLFORM_CONTENTTYPE, "application/octet-stream",
               CURLFORM_FILENAME, basename(call->converted),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "freq",
               CURLFORM_COPYCONTENTS, freq.str().c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "start_time",
               CURLFORM_COPYCONTENTS, boost::lexical_cast<std::string>(call->start_time).c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "stop_time",
               CURLFORM_COPYCONTENTS, boost::lexical_cast<std::string>(call->stop_time).c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "call_length",
               CURLFORM_COPYCONTENTS, call_length.str().c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "talkgroup_num",
               CURLFORM_COPYCONTENTS, boost::lexical_cast<std::string>(call->talkgroup).c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "emergency",
               CURLFORM_COPYCONTENTS, boost::lexical_cast<std::string>(call->emergency).c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "api_key",
               CURLFORM_COPYCONTENTS, call->api_key.c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "source_list",
               CURLFORM_COPYCONTENTS, source_list.str().c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "freq_list",
               CURLFORM_COPYCONTENTS, freq_list.str().c_str(),
               CURLFORM_END);

  curl = curl_easy_init();
  multi_handle = curl_multi_init();

  /* initialize custom header list (stating that Expect: 100-continue is not wanted */
  headerlist = curl_slist_append(headerlist, "Expect:");
  if (curl && multi_handle) {
    std::string url = call->upload_server + "/" + call->short_name + "/upload";

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
      int rc; /* select() return code */
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
        struct timeval wait = { 0, 100 * 1000 }; /* 100ms */
        rc = select(0, NULL, NULL, NULL, &wait);
      }
      else {
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
      stat(call->converted, &file_info);

      BOOST_LOG_TRIVIAL(info) <<"[" << call->short_name <<  "]\tTG: " << call->talkgroup << "\tFreq: " << FormatFreq(call->freq) << "\tOpenMHz Upload Success - file size: " << file_info.st_size;;
      return 0;
    }
  }
  BOOST_LOG_TRIVIAL(error) << "[" << call->short_name << "]\tTG: " << call->talkgroup << "\tFreq: " << FormatFreq(call->freq) << "\tOpenMHz Upload Error: " << response_buffer;
  return 1;
}
