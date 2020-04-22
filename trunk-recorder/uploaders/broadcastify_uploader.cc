#include "uploader.h"
#include "broadcastify_uploader.h"

CURLcode BroadcastifyUploader::upload_audio_file(std::string converted, std::string url) {
  struct stat file_info;

  /* get the file size of the local file */
  stat(converted.c_str(), &file_info);

  FILE * audio = fopen(converted.c_str(), "rb");

  // Make sure we have something to read.
  if (!audio) {
    BOOST_LOG_TRIVIAL(info) << "Error opening file " << converted;
    return CURLE_READ_ERROR;
  }

  CURL *curl;
  CURLcode res;

  struct curl_slist *headers = NULL;

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "TrunkRecorder1.0");

    /* enable uploading */
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    /* HTTP PUT please */
    curl_easy_setopt(curl, CURLOPT_PUT, 1L);

    /* specify target URL, and note that this URL should include a file
       name, not only a directory */
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    /* now specify which file to upload */
    curl_easy_setopt(curl, CURLOPT_READDATA, audio);

    /* provide the size of the upload, we specially typecast the value
       to curl_off_t since we must be sure to use the correct data size */
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                     (curl_off_t)file_info.st_size);

    headers = curl_slist_append(headers, "Content-Type: audio/aac");
    /* Expect: 100-continue is not wanted */
    headers = curl_slist_append(headers, "Expect:");
    /* Transfer-Encoding: chunked is not wanted */
    headers = curl_slist_append(headers, "Transfer-Encoding:");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);

    /* always cleanup */
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }

  fclose(audio);

  return res;
}

int BroadcastifyUploader::upload(struct call_data_t *call) {
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
               CURLFORM_COPYNAME, "metadata",
               CURLFORM_FILE, call->status_filename,
               CURLFORM_CONTENTTYPE, "application/json",
               CURLFORM_END);

  /* Fill in the filename field */
  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "filename",
               CURLFORM_COPYCONTENTS, basename(call->converted),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "callDuration",
               CURLFORM_COPYCONTENTS, std::to_string(call->length).c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "systemId",
               CURLFORM_COPYCONTENTS, std::to_string(call->bcfy_system_id).c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "apiKey",
               CURLFORM_COPYCONTENTS, call->bcfy_api_key.c_str(),
               CURLFORM_END);

  curl = curl_easy_init();
  multi_handle = curl_multi_init();

  /* initialize custom header list (stating that Expect: 100-continue is not wanted */
  headerlist = curl_slist_append(headerlist, "Expect:");
  if (curl && multi_handle) {
    /* what URL that receives this POST */
    curl_easy_setopt(curl, CURLOPT_URL, call->bcfy_calls_server.c_str());

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

    if (res != CURLM_OK || response_code != 200) {
      BOOST_LOG_TRIVIAL(error) << "[" << call->short_name << "]\tTG: " << call->talkgroup << "\tFreq: " << FormatFreq(call->freq) << "\tBroadcastify Metadata Upload Error: " << response_buffer;
      return 1;
    }

    std::size_t spacepos = response_buffer.find(' ');
    if (spacepos < 1) {
      BOOST_LOG_TRIVIAL(error) << "[" << call->short_name << "]\tTG: " << call->talkgroup << "\tFreq: " << FormatFreq(call->freq) << "\tBroadcastify Metadata Upload Error: " << response_buffer;
      return 1;
    }

    std::string code = response_buffer.substr(0, spacepos);
    std::string message = response_buffer.substr(spacepos + 1);

    if (code != "0") {
      BOOST_LOG_TRIVIAL(error) << "[" << call->short_name << "]\tTG: " << call->talkgroup << "\tFreq: " << FormatFreq(call->freq) << "\tBroadcastify Metadata Upload Error: " << message;
      return 1;
    }

    CURLcode audio_error = this->upload_audio_file(call->converted, message);

    if (audio_error) {
      BOOST_LOG_TRIVIAL(error) << "[" << call->short_name << "]\tTG: " << call->talkgroup << "\tFreq: " << FormatFreq(call->freq) << "\tBroadcastify Audio Upload Error: " << curl_easy_strerror(audio_error);
      return 1;
    }

    struct stat file_info;
    stat(call->converted, &file_info);

    BOOST_LOG_TRIVIAL(info) <<"[" << call->short_name <<  "]\tTG: " << call->talkgroup << "\tFreq: " << FormatFreq(call->freq) << "\tBroadcastify Upload Success - file size: " << file_info.st_size;
    return 0;
  } else {
    return 1;
  }
}