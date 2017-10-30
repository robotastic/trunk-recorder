#ifndef STAT_SOCKET_H
#define STAT_SOCKET_H

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <time.h>

#include "../source.h"
#include "../config.h"
#include "../systems/system.h"


// This header pulls in the WebSocket++ abstracted thread support that will
// select between boost::thread and std::thread based on how the build system
// is configured.
#include <websocketpp/common/thread.hpp>


typedef websocketpp::client<websocketpp::config::asio_client> client;
//typedef websocketpp::retry_client_endpoint<websocketpp::retry_config<websocketpp::config::asio_client>> client;
typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;

class stat_socket {
  public:
  stat_socket();
  void send_stat(std::string val);
  void poll_one();
  void telemetry_loop();
  void on_fail(websocketpp::connection_hdl);
  void on_close(websocketpp::connection_hdl);
  void on_open(websocketpp::connection_hdl);
  void open_stat(const std::string & uri);
  bool is_open();
  void send_status(std::vector<Call *>calls, Config config);
  void send_config(std::vector<Source *> sources, std::vector<System *> systems, Config config);
  void send_sys_rates(std::vector<System *>systems, float timeDiff, Config config) ;

private:
    void reopen_stat();
    client m_client;
    websocketpp::connection_hdl m_hdl;
    websocketpp::lib::mutex m_lock;
    std::string remote_uri;
    int retry_attempt;
    time_t reconnect_time;
    bool m_reconnect;
    bool m_open;
    bool m_done;
};

#endif
