#include <vector>
#include <curl/curl.h>
#include "call_uploader.h"
#include "../formatter.h"

void build_call_request(struct call_data_t *call, boost::asio::streambuf& request_) {
  // boost::asio::streambuf request_;
  // std::string server = "api.openmhz.com";
  // std::string path =  "/upload";
  std::string boundary("MD5_0be63cda3bf42193e4303db2c5ac3138");


  std::string form_name("call");
  std::string form_filename(call->converted);
  std::ostringstream conv;
  std::ifstream file(call->converted, std::ios::binary);

  // Make sure we have something to read.
  if (!file.is_open()) {
    BOOST_LOG_TRIVIAL(info) << "Error opening file ";

    // throw (std::exception("Could not open file."));
  }

  // Copy contents "as efficiently as possible".


  // ------------------------------------------------------------------------
  // Create Disposition in a stringstream, because we need Content-Length...
  std::ostringstream oss;
  oss << "--" << boundary << "\r\n";
  oss << "Content-Disposition: form-data; name=\"" << form_name << "\"; filename=\"" << form_filename << "\"\r\n";

  // oss << "Content-Type: text/plain\r\n";
  oss << "Content-Type: application/octet-stream\r\n";
  oss << "Content-Transfer-Encoding: binary\r\n";
  oss << "\r\n";

  oss << file.rdbuf();
  file.close();
  oss.clear();

  std::ostringstream  source_list;
  source_list << std::fixed << std::setprecision(2);
  source_list << "[";

  if (call->source_count != 0) {
    for (int i = 0; i < call->source_count; i++) {
      source_list << "{ \"pos\": " << std::setprecision(2) << call->source_list[i].position << ", \"src\": " << std::setprecision(0) << call->source_list[i].source << " }";

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
    for (int i = 0; i < call->freq_count; i++) {
      freq_list << "{ \"pos\": " << std::setprecision(2) << call->freq_list[i].position << ", \"freq\": " << std::setprecision(0) << call->freq_list[i].freq << ", \"len\": " << call->freq_list[i].total_len << ", \"errors\": " << call->freq_list[i].error_count << ", \"spikes\": " << call->freq_list[i].spike_count << " }";

      if (i < (call->freq_count - 1)) {
        freq_list << ", ";
      } else {
        freq_list << "]";
      }
    }
  } else {
    freq_list << "]";
  }

  conv << std::fixed << std::setprecision(0);
  conv << call->freq;
  add_post_field(oss, "freq",          conv.str(),       boundary);
  conv.clear();
  conv.str("");
  add_post_field(oss, "start_time",    boost::lexical_cast<std::string>(call->start_time), boundary);
  add_post_field(oss, "stop_time",     boost::lexical_cast<std::string>(call->stop_time),  boundary);
  conv << std::fixed << std::setprecision(0);
  conv << call->length;
  add_post_field(oss, "call_length",          conv.str(),       boundary);
  conv.clear();
  conv.str("");
  add_post_field(oss, "talkgroup_num", boost::lexical_cast<std::string>(call->talkgroup),  boundary);
  add_post_field(oss, "emergency",     boost::lexical_cast<std::string>(call->emergency),  boundary);
  add_post_field(oss, "api_key",       call->api_key,                                      boundary);
  add_post_field(oss, "source_list",   source_list.str(),                                        boundary);
  add_post_field(oss, "freq_list",     freq_list.str(),                                          boundary);
  oss << "\r\n--" << boundary << "--\r\n";
  const std::string& body_str(oss.str());

  // oss.clear();
  // oss.flush();
  // ------------------------------------------------------------------------


  std::ostream post_stream(&request_);

  post_stream << "POST " << call->path << "" << " HTTP/1.1\r\n";
  post_stream << "Content-Type: multipart/form-data; boundary=" << boundary << "\r\n";
  post_stream << "User-Agent: TrunkRecorder1.0\r\n";
  post_stream << "Host: " << call->hostname << "\r\n"; // The domain name of the
                                                       // server (for virtual
                                                       // hosting), mandatory
                                                       // since HTTP/1.1
  post_stream << "Accept: */*\r\n";
  post_stream << "Connection: Close\r\n";
  post_stream << "Cache-Control: no-cache\r\n";
  post_stream << "Content-Length: " << body_str.size() << "\r\n"; // size_of_stream(oss)
                                                                  // << "\r\n";
  post_stream << "\r\n";

  post_stream << body_str;
}

static std::string response_body;

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string*) userp)->append((char*) contents, size * nmemb);
  return size * nmemb;
}

void make_call_upload_request(struct call_data_t *call) {
  struct stat file_info;

  /* get the file size of the local file */
  stat(call->converted, &file_info);

  FILE * audio = fopen(call->converted, "rb");

  // Make sure we have something to read.
  if (!audio) {
    BOOST_LOG_TRIVIAL(info) << "Error opening file " << call->converted;
    return;
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
    curl_easy_setopt(curl, CURLOPT_URL, call->bcfy_calls_server.c_str());

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
    /* Check for errors */
    if (res == CURLE_OK) {
      BOOST_LOG_TRIVIAL(info) <<"[" << call->short_name <<  "]\tTG: " << call->talkgroup << "\tFreq: " << FormatFreq(call->freq) << "\tBroadcastify Audio Upload Success";
    } else {
      BOOST_LOG_TRIVIAL(error) <<"[" << call->short_name <<  "]\tTG: " << call->talkgroup << "\tFreq: " << FormatFreq(call->freq) << "\tBroadcastify Audio Upload Error";
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }

    /* always cleanup */
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }

  fclose(audio);
}

void upload_to_broadcastify(struct call_data_t *call) {
  std::string callDuration = std::to_string(call->length);
  std::string systemId = std::to_string(call->bcfy_system_id);

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
               CURLFORM_COPYCONTENTS, callDuration.c_str(),
               CURLFORM_END);

  curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "systemId",
               CURLFORM_COPYCONTENTS, systemId.c_str(),
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

    if (res == CURLM_OK) {
      std::size_t spacepos = response_buffer.find(' ');
      if (spacepos > 0) {
        std::string code = response_buffer.substr(0, spacepos);
        std::string message = response_buffer.substr(spacepos + 1);

        if (code == "0") {
          call->bcfy_calls_server = message;
          make_call_upload_request(call);
        } else {
          BOOST_LOG_TRIVIAL(error) << "[" << call->short_name << "]\tTG: " << call->talkgroup << "\tFreq: " << FormatFreq(call->freq) << "\tBroadcastify Metadata Upload Error: " << message;
        }
      } else {
        BOOST_LOG_TRIVIAL(error) << "[" << call->short_name << "]\tTG: " << call->talkgroup << "\tFreq: " << FormatFreq(call->freq) << "\tBroadcastify Metadata Upload Error: " << response_buffer;
      }
    } else {
      BOOST_LOG_TRIVIAL(error) << "[" << call->short_name << "]\tTG: " << call->talkgroup << "\tFreq: " << FormatFreq(call->freq) << "\tBroadcastify Metadata Upload Error";
    }
  }
}

void convert_upload_call(call_data_t *call_info, server_data_t *server_info) {
  char shell_command[400];

  //int nchars = snprintf(shell_command, 400, "nice -n -10 ffmpeg -y -i %s  -c:a libfdk_aac -b:a 32k -filter:a \"volume=15db\" -filter:a loudnorm -cutoff 18000 -hide_banner -loglevel panic %s ", call_info->filename, call_info->converted);
  //int nchars = snprintf(shell_command, 400, "ffmpeg -y -i %s  -c:a libfdk_aac -b:a 32k -filter:a \"volume=15db\" -filter:a loudnorm  -hide_banner -loglevel panic %s ", call_info->filename, call_info->converted);
  //int nchars = snprintf(shell_command, 400, "cd %s && fdkaac -S -b16 --raw-channels 1 --raw-rate 8000 %s", call_info->file_path, call_info->filename);
  //hints from here https://github.com/nu774/fdkaac/issues/5 on how to pipe between the 2
  int nchars = snprintf(shell_command, 400, "sox %s -t wav - --norm=-3 | fdkaac --ignorelength -b 8000 -o %s -", call_info->filename, call_info->converted);

  if (nchars >= 400) {
    BOOST_LOG_TRIVIAL(error) << "Call Uploader: Path longer than 400 characters";
  }

  // BOOST_LOG_TRIVIAL(info) << "Converting: " << call_info->converted << "\n";
  // BOOST_LOG_TRIVIAL(info) <<"Command: " << shell_command << "\n";
  int forget = system(shell_command);
  //int rc = system(shell_command);

  // BOOST_LOG_TRIVIAL(info) << "Finished converting\n";

  if (call_info->bcfy_calls_server != "" && call_info->bcfy_api_key != "" && call_info->bcfy_system_id > 0) {
    upload_to_broadcastify(call_info);
  }

  if (call_info->upload_server == "") {
    if (!call_info->audio_archive) {
      unlink(call_info->filename);
      unlink(call_info->converted);
    }
    return;
  }

  boost::asio::streambuf request_;

  build_call_request(call_info, request_);

  // BOOST_LOG_TRIVIAL(info) << "Finished Build Call Request\n";

  size_t req_size = request_.size();

  int error;

  if (call_info->scheme == "http") {
    error = http_upload(server_info, request_);
  }

  if (call_info->scheme == "https") {
    error = https_upload(server_info, request_);
  }

  if (!error) {
    BOOST_LOG_TRIVIAL(info) <<"[" << call_info->short_name <<  "]\tTG: " << call_info->talkgroup << "\tFreq: " << FormatFreq(call_info->freq) << "\tHTTP(S) Upload Success - file size: " << req_size;
    if (!call_info->audio_archive) {
      unlink(call_info->filename);
      unlink(call_info->converted);
    }
  } else {
    BOOST_LOG_TRIVIAL(error) <<"[" << call_info->short_name <<  "]\tTG: " << call_info->talkgroup << "\tFreq: " << FormatFreq(call_info->freq) << "\tHTTP(S) Upload Error - file size: " << req_size;
  }

  // BOOST_LOG_TRIVIAL(info) << "Try to clear: " << req_size;
  request_.consume(req_size);
}

void* upload_call_thread(void *thread_arg) {
  call_data_t   *call_info;
  server_data_t *server_info = new server_data_t;

  pthread_detach(pthread_self());

  call_info                  = static_cast<call_data_t *>(thread_arg);
  server_info->upload_server = call_info->upload_server;
  server_info->scheme        = call_info->scheme;
  server_info->hostname      = call_info->hostname;
  server_info->port          = call_info->port;
  server_info->path          = call_info->path;

  /*boost::filesystem::path m4a(call_info->filename);
     m4a = m4a.replace_extension(".m4a");
     const std::string& m4a_str(m4a.string());
     strcpy(call_info->converted, m4a_str.c_str());*/

  convert_upload_call(call_info, server_info);

  delete(server_info);
  delete(call_info);
  return NULL;

  // pthread_exit(NULL);
}

void send_call(Call *call, System *sys, Config config) {
  // struct call_data_t *call_info = (struct call_data_t *) malloc(sizeof(struct
  // call_data_t));
  call_data_t *call_info = new call_data_t;
  pthread_t    thread;


  boost::regex  ex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
  boost::cmatch what;

  strcpy(call_info->filename,  call->get_filename());
  strcpy(call_info->converted, call->get_converted_filename());
  strcpy(call_info->status_filename, call->get_status_filename());
  strcpy(call_info->file_path, call->get_path());

  if (regex_match(config.upload_server.c_str(), what, ex)) {
    // from: http://www.zedwood.com/article/cpp-boost-url-regex
    call_info->upload_server = config.upload_server;
    call_info->scheme        = std::string(what[1].first, what[1].second);
    call_info->hostname      = std::string(what[2].first, what[2].second);
    call_info->port          = std::string(what[3].first, what[3].second);
    call_info->path          = std::string(what[4].first, what[4].second);
  }
  if (regex_match(config.bcfy_calls_server.c_str(), what, ex)) {
    call_info->bcfy_calls_server = config.bcfy_calls_server;
  }

  // Exit if neither of our above URLs were valid
  if (call_info->upload_server.empty() && call_info->bcfy_calls_server.empty()) {
    BOOST_LOG_TRIVIAL(info) << "Unable to parse Server URL\n";
    return;
  }

  // std::cout << "Setting up thread\n";
  std::vector<Call_Source> source_list = call->get_source_list();
  Call_Freq   *freq_list   = call->get_freq_list();
  //Call_Error  *error_list  = call->get_error_list();
  call_info->talkgroup        = call->get_talkgroup();
  call_info->freq             = call->get_freq();
  call_info->encrypted        = call->get_encrypted();
  call_info->emergency        = call->get_emergency();
  call_info->tdma_slot        = call->get_tdma_slot();
  call_info->phase2_tdma      = call->get_phase2_tdma();
  call_info->error_list_count = call->get_error_list_count();
  call_info->source_count     = call->get_source_count();
  call_info->freq_count       = call->get_freq_count();
  call_info->start_time       = call->get_start_time();
  call_info->stop_time        = call->get_stop_time();
  call_info->length           = (int) call->get_final_length();
  call_info->api_key          = sys->get_api_key();
  call_info->bcfy_api_key     = sys->get_bcfy_api_key();
  call_info->bcfy_system_id   = sys->get_bcfy_system_id();
  call_info->short_name       = sys->get_short_name();
  call_info->audio_archive    = sys->get_audio_archive();
  std::stringstream ss;
  ss << "/" << sys->get_short_name() << "/upload";
  call_info->path = ss.str();

  // std::cout << "Upload - Scheme: " << call_info->scheme << " Hostname: " <<
  // call_info->hostname << " Port: " << call_info->port << " Path: " <<
  // call_info->path << "\n";

  for (int i = 0; i < call_info->source_count; i++) {
    call_info->source_list.push_back(source_list[i]);
  }

  for (int i = 0; i < call_info->freq_count; i++) {
    call_info->freq_list[i] = freq_list[i];
  }

  int rc = pthread_create(&thread, NULL, upload_call_thread, (void *)call_info);

  // pthread_detach(thread);

  if (rc) {
    printf("ERROR; return code from pthread_create() is %d", rc);
  }
}
