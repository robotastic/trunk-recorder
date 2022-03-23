#include "../../trunk-recorder/plugin_manager/plugin_api.h"
#include "../../trunk-recorder/recorders/recorder.h"
#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS
#include <boost/foreach.hpp>
#include <boost/asio.hpp>

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

  int parse_config(boost::property_tree::ptree &cfg) {
    BOOST_FOREACH (boost::property_tree::ptree::value_type &node, cfg.get_child("streams")) {
      stream_t stream;
      stream.TGID = node.second.get<unsigned long>("TGID");
      stream.address = node.second.get<std::string>("address");
      stream.port = node.second.get<long>("port");
      stream.remote_endpoint = ip::udp::endpoint(ip::address::from_string(stream.address), stream.port);
      stream.sendTGID = node.second.get<bool>("sendTGID",false);
      stream.tcp = node.second.get<bool>("useTCP",false);
      stream.short_name = node.second.get<std::string>("shortName", "");
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
            if (stream.sendTGID==true){
              //prepend 4 byte long tgid to the audio data
              boost::array<mutable_buffer, 2> buf1 = {
                buffer(&TGID,4),
                buffer(samples, sampleCount*2)
              };
              if(stream.tcp==true){
                stream.tcp_socket->send(buf1);
              }
              else{
                my_socket.send_to(buf1, stream.remote_endpoint, 0, error);
              }
            }
            else{
              //just send the audio data
              if(stream.tcp == true){
                stream.tcp_socket->send(buffer(samples, sampleCount*2));
              }
              else{
                my_socket.send_to(buffer(samples, sampleCount*2), stream.remote_endpoint, 0, error);
              }
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
        io_service my_tcp_io_service;
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
