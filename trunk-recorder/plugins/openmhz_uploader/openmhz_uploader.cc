#include <curl/curl.h>
#include <time.h>
#include <vector>

#include "../../call_concluder/call_concluder.h"
#include "../plugin_api.h"
#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS
#include <gr_blocks/decoder_wrapper.h>

struct Openmhz_System_Key {
  std::string api_key;
  std::string short_name;
};

struct Openmhz_Uploader_Data {
  std::vector<Openmhz_System_Key> keys;
  std::string openmhz_server;
};

class Openmhz_Uploader : public Plugin_Api {
  //float aggr_;
  //my_plugin_aggregator() : aggr_(0) {}
  Openmhz_Uploader_Data data;

public:
  std::string get_api_key(std::string short_name) {
    for (std::vector<Openmhz_System_Key>::iterator it = data.keys.begin(); it != data.keys.end(); ++it) {
      Openmhz_System_Key key = *it;
      if (key.short_name == short_name){
        return key.api_key;
      }
    }
    return "";
  }
  static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
  }
  int upload(Call_Data_t call_info) {
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

    char formattedTalkgroup[62];
    snprintf(formattedTalkgroup, 61, "%c[%dm%10ld%c[0m", 0x1B, 35, call_info.talkgroup, 0x1B);
    std::string talkgroup_display = boost::lexical_cast<std::string>(formattedTalkgroup);
    time_t start_time = call_info.start_time;

    std::string api_key = get_api_key(call_info.short_name);
    BOOST_LOG_TRIVIAL(info) << "using api key: " << api_key;
    if (api_key.size()==0){
      BOOST_LOG_TRIVIAL(error) << "[" << call_info.short_name << "]\tTG: " << talkgroup_display << "\t " << std::put_time(std::localtime(&start_time), "%c %Z") << "\tOpenMHz Upload failed, API Key not found in config for shortName";
      return 2;
    }

    if (call_info.call_source_list.size() != 0) {
      for (int i = 0; i < call_info.call_source_list.size(); i++) {
        source_list << "{ \"pos\": " << std::setprecision(2) << call_info.call_source_list[i].position << ", \"src\": " << std::setprecision(0) << call_info.call_source_list[i].source << " }";

        if (i < (call_info.call_source_list.size() - 1)) {
          source_list << ", ";
        } else {
          source_list << "]";
        }
      }
    } else {
      source_list << "]";
    }

    BOOST_LOG_TRIVIAL(error) << "Got source list: " << source_list.str();
    CURL *curl;
    CURLMcode res;
    CURLM *multi_handle;
    int still_running = 0;
    std::string response_buffer;
    freq_string = freq.str();

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
                 CURLFORM_COPYCONTENTS, api_key.c_str(),
                 CURLFORM_END);

    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "source_list",
                 CURLFORM_COPYCONTENTS, source_list_string.c_str(),
                 CURLFORM_END);
      curl_formadd(&formpost,
               &lastptr,
               CURLFORM_COPYNAME, "freq_list",
               CURLFORM_COPYCONTENTS, "[]",
               CURLFORM_END);

    curl = curl_easy_init();
    multi_handle = curl_multi_init();

    /* initialize custom header list (stating that Expect: 100-continue is not wanted */
    headerlist = curl_slist_append(headerlist, "Expect:");
    if (curl && multi_handle) {
      std::string url = data.openmhz_server + "/" + call_info.short_name + "/upload";

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

        BOOST_LOG_TRIVIAL(info) << "[" << call_info.short_name << "]\tTG: " << talkgroup_display << "\t " << std::put_time(std::localtime(&start_time), "%c %Z") << "\tOpenMHz Upload Success - file size: " << file_info.st_size;
        ;
        return 0;
      }
    }
    BOOST_LOG_TRIVIAL(error) << "[" << call_info.short_name << "]\tTG: " << talkgroup_display << "\t " << std::put_time(std::localtime(&start_time), "%c %Z") << "\tOpenMHz Upload Error: " << response_buffer;
    return 1;
  }

  int call_end(Call_Data_t call_info) {
    return upload(call_info);
  }

  int parse_config(boost::property_tree::ptree &cfg) {

    BOOST_FOREACH (boost::property_tree::ptree::value_type &node, cfg.get_child("systems")) {
      boost::optional<boost::property_tree::ptree &> openmhz_exists = node.second.get_child_optional("apiKey");
      if (openmhz_exists) {
        Openmhz_System_Key key;
        key.api_key = node.second.get<std::string>("apiKey", "");
        key.short_name = node.second.get<std::string>("shortName", "");
        BOOST_LOG_TRIVIAL(info) << "Uploading calls for: " << key.short_name;
        this->data.keys.push_back(key);
      }
    }
    this->data.openmhz_server = cfg.get<std::string>("uploadServer", "");
    BOOST_LOG_TRIVIAL(info) << "OpenMHz Server: " << this->data.openmhz_server;

  // from: http://www.zedwood.com/article/cpp-boost-url-regex
  boost::regex ex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
  boost::cmatch what;

    if (!regex_match(this->data.openmhz_server.c_str(), what, ex)) {
      BOOST_LOG_TRIVIAL(error) << "Unable to parse Server URL\n";
      return 1;
    } else if (this->data.keys.size() ==0){
      BOOST_LOG_TRIVIAL(error) << "OpenMHz Server set, but no Systems are configured\n";
      return 1;
    }

    return 0;
  }

  /*
 int init(Config *config, std::vector<Source *> sources, std::vector<System *> systems) { return 0; }
   int start() { return 0; }
   int stop() { return 0; }
   int poll_one() { return 0; }
   int signal(long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder) { return 0; }
   int audio_stream(Recorder *recorder, float *samples, int sampleCount) { return 0; }
   int call_start(Call *call) { return 0; }
   int calls_active(std::vector<Call *> calls) { return 0; }
   int setup_recorder(Recorder *recorder) { return 0; }
   int setup_system(System *system) { return 0; }
   int setup_systems(std::vector<System *> systems) { return 0; }
   int setup_sources(std::vector<Source *> sources) { return 0; }
   int setup_config(std::vector<Source *> sources, std::vector<System *> systems) { return 0; }
   int system_rates(std::vector<System *> systems, float timeDiff) { return 0; }
*/
  // Factory method
  static boost::shared_ptr<Openmhz_Uploader> create() {
    return boost::shared_ptr<Openmhz_Uploader>(
        new Openmhz_Uploader());
  }
};

BOOST_DLL_ALIAS(
    Openmhz_Uploader::create, // <-- this function is exported with...
    create_plugin             // <-- ...this alias name
)
