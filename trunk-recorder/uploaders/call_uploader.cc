#include "call_uploader.h"


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

void convert_upload_call(call_data_t *call_info, server_data_t *server_info) {
  char shell_command[400];

  //int nchars = snprintf(shell_command, 400, "nice -n -10 ffmpeg -y -i %s  -c:a libfdk_aac -b:a 32k -filter:a \"volume=15db\" -filter:a loudnorm -cutoff 18000 -hide_banner -loglevel panic %s ", call_info->filename, call_info->converted);
  int nchars = snprintf(shell_command, 400, "ffmpeg -y -i %s  -c:a libfdk_aac -b:a 32k -filter:a \"volume=15db\" -filter:a loudnorm -cutoff 18000 -hide_banner -loglevel panic %s ", call_info->filename, call_info->converted);

  if (nchars >= 400) {
    BOOST_LOG_TRIVIAL(error) << "Call Uploader: Path longer than 400 charecters";
  }

  // BOOST_LOG_TRIVIAL(info) << "Converting: " << call_info->converted << "\n";
  // BOOST_LOG_TRIVIAL(info) <<"Command: " << shell_command << "\n";
  system(shell_command);
  //int rc = system(shell_command);

  // BOOST_LOG_TRIVIAL(info) << "Finished converting\n";

  boost::asio::streambuf request_;

  build_call_request(call_info, request_);

  // BOOST_LOG_TRIVIAL(info) << "Finished Build Call Request\n";

  size_t req_size = request_.size();

  if (call_info->scheme == "http") {
    BOOST_LOG_TRIVIAL(info) <<"[" << call_info->short_name <<  "]\tTG: " << call_info->talkgroup << "\tFreq: " << call_info->freq  << "\tHTTP Upload result: " << http_upload(server_info, request_);
  }

  if (call_info->scheme == "https") {
    int error = https_upload(server_info, request_);

    if (!error) {
      BOOST_LOG_TRIVIAL(info) <<"[" << call_info->short_name <<  "]\tTG: " << call_info->talkgroup << "\tFreq: " << call_info->freq << "\tHTTPS Upload Success - file size: " << req_size;
      if (!call_info->audio_archive) {
        unlink(call_info->filename);
        unlink(call_info->converted);
      }
    } else {
      BOOST_LOG_TRIVIAL(error) <<"[" << call_info->short_name <<  "]\tTG: " << call_info->talkgroup << "\tFreq: " << call_info->freq << "\tHTTPS Upload Error - file size: " << req_size;

    }
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

  if (regex_match(config.upload_server.c_str(), what, ex))
  {
    // from: http://www.zedwood.com/article/cpp-boost-url-regex
    call_info->upload_server = config.upload_server;
    call_info->scheme        = std::string(what[1].first, what[1].second);
    call_info->hostname      = std::string(what[2].first, what[2].second);
    call_info->port          = std::string(what[3].first, what[3].second);
    call_info->path          = std::string(what[4].first, what[4].second);

    // std::cout << "Upload - Scheme: " << call_info->scheme << " Hostname: " <<
    // call_info->hostname << " Port: " << call_info->port << " Path: " <<
    // call_info->path << "\n";
    strcpy(call_info->filename,  call->get_filename());
    strcpy(call_info->converted, call->get_converted_filename());
  } else {
    // std::cout << "Unable to parse Server URL\n";
    return;
  }

  // std::cout << "Setting up thread\n";
  Call_Source *source_list = call->get_source_list();
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
  call_info->api_key          = sys->get_api_key();
  call_info->short_name       = sys->get_short_name();
  call_info->audio_archive    = sys->get_audio_archive();
  std::stringstream ss;
  ss << "/" << sys->get_short_name() << "/upload";
  call_info->path = ss.str();

  // std::cout << "Upload - Scheme: " << call_info->scheme << " Hostname: " <<
  // call_info->hostname << " Port: " << call_info->port << " Path: " <<
  // call_info->path << "\n";

  for (int i = 0; i < call_info->source_count; i++) {
    call_info->source_list[i] = source_list[i];
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
