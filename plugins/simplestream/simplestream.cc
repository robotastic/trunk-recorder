#include "../../trunk-recorder/plugin_manager/plugin_api.h"
#include "../../trunk-recorder/recorders/recorder.h"
#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS
#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>

using namespace boost::asio;

typedef struct plugin_t plugin_t;
typedef struct stream_t stream_t;
std::vector<stream_t> streams;
io_service my_tcp_io_service;
long max_tcp_index = 0;


struct plugin_t {
  Config* config;
};

struct stream_t {
  unsigned long TGID;
  long tcp_index;
  std::string address;
  std::string short_name;
  long port;
  ip::udp::endpoint remote_endpoint;
  ip::tcp::socket *tcp_socket;
  bool sendTGID = false;
  bool sendJSON = false;
  bool tcp = false;
};

class Simple_Stream : public Plugin_Api {
  typedef boost::asio::io_service io_service;
  io_service my_io_service;
  ip::udp::endpoint remote_endpoint;
  ip::udp::socket my_socket{my_io_service};
  public:
  
  Simple_Stream(){
      
  }

 int parse_config(json config_data) {
    for (json element : config_data["streams"]) {
      stream_t stream;
      stream.TGID = element["TGID"];
      stream.address = element["address"];
      stream.port = element["port"];
      stream.remote_endpoint = ip::udp::endpoint(ip::address::from_string(stream.address), stream.port);
      stream.sendTGID = element.value("sendTGID",false);
      stream.sendJSON = element.value("sendJSON",false);
      stream.tcp = element.value("useTCP",false);
      stream.short_name = element.value("shortName", "");
      BOOST_LOG_TRIVIAL(info) << "simplestreamer will stream audio from TGID " <<stream.TGID << " on System " <<stream.short_name << " to " << stream.address <<" on port " << stream.port << " tcp is "<<stream.tcp;
      streams.push_back(stream);
    }
    return 0;
  }
  
  int audio_stream(Call *call, Recorder *recorder, int16_t *samples, int sampleCount){
    int recorder_id = recorder->get_num();
    boost::system::error_code error;
    BOOST_FOREACH (auto& stream, streams){
      if (0==stream.short_name.compare(call->get_system()->get_short_name()) || (0==stream.short_name.compare(""))){ //Check if shortName matches or is not specified
        std::vector<unsigned long> patched_talkgroups = call->get_system()->get_talkgroup_patch(call->get_talkgroup());
        if (patched_talkgroups.size() == 0){
          patched_talkgroups.push_back(call->get_talkgroup());
        }
        BOOST_FOREACH (auto& TGID, patched_talkgroups){
          if ((TGID==stream.TGID || stream.TGID==0)){  //setting TGID to 0 in the config file will stream everything
            BOOST_LOG_TRIVIAL(debug) << "got " <<sampleCount <<" samples - " <<sampleCount*2<<" bytes from recorder "<<recorder_id<<" for TGID "<<TGID;
            json json_object;
            std::string json_string;
            std::vector<boost::asio::const_buffer> send_buffer;
            if (stream.sendJSON==true){
              //create JSON metadata
              json_object = {
                 {"src", call->get_current_source_id()},
                 {"talkgroup", TGID},
                 {"patched_talkgroups",patched_talkgroups},
                 {"freq", call->get_freq()},
                 {"short_name", call->get_short_name()},
                 {"audio_sample_rate",recorder->get_wav_hz()},
              };
              json_string = json_object.dump();
              uint32_t json_length = json_string.length();  //determine length in bytes
              //BOOST_LOG_TRIVIAL(debug) << "json_length is " <<json_length <<" bytes";
              send_buffer.push_back(buffer(&json_length,4));  //prepend length of the json data
              send_buffer.push_back(buffer(json_string));  //prepend json data
              //BOOST_LOG_TRIVIAL(debug) << "json_string is " <<json_string;
            }
            else if (stream.sendTGID==true){
              send_buffer.push_back(buffer(&TGID,4));  //prepend 4 byte long tgid to the audio data
            }
            send_buffer.push_back(buffer(samples, sampleCount*2));
            if(stream.tcp == true){
              stream.tcp_socket->send(send_buffer);
            }
            else{
              my_socket.send_to(send_buffer, stream.remote_endpoint, 0, error);
            }
          }
        }
      }
    }
    return 0;
  }
  
  int start(){
    BOOST_FOREACH (auto& stream, streams){
      if (stream.tcp == true){
        ip::tcp::socket *my_tcp_socket = new ip::tcp::socket{my_tcp_io_service};
        stream.tcp_socket = my_tcp_socket;
        stream.tcp_socket->connect(ip::tcp::endpoint( boost::asio::ip::address::from_string(stream.address), stream.port ));
      }
    }
    my_socket.open(ip::udp::v4());
    return 0;
  }
  
  int stop(){
    BOOST_FOREACH (auto& stream, streams){
      if (stream.tcp == true){
        stream.tcp_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        stream.tcp_socket->close();
      }
    }
    my_socket.close();
    return 0;
  }

  static boost::shared_ptr<Simple_Stream> create() {
    return boost::shared_ptr<Simple_Stream>(
        new Simple_Stream());
  }
};

BOOST_DLL_ALIAS(
    Simple_Stream::create, // <-- this function is exported with...
    create_plugin             // <-- ...this alias name
)
