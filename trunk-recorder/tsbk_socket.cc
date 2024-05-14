#include "tsbk_socket.h"

TSBK_Socket::TSBK_Socket(std::string server_addr, int port) : m_server(""), m_port(5678) {
    m_server = server_addr;
    m_port = port;

    
    m_servlen = sizeof(m_servaddr);

    if ( (m_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        BOOST_LOG_TRIVIAL(error) << "Failure in creating TSBK UDP socket.";
        exit(EXIT_FAILURE);
    }

    m_servaddr.sin_family = AF_INET;
    m_servaddr.sin_port = htons(m_port);
    char* c_server = const_cast<char*>(m_server.c_str());
    m_servaddr.sin_addr.s_addr = inet_addr(c_server);
}

void TSBK_Socket::send_tsbk(boost::dynamic_bitset<> &tsbk, System *system) {

    if (m_server.empty())
      return;

    boost::property_tree::ptree root;

    root.put("version", "V1");
    root.put("type", "tsbk");
    root.put("sysName", system->get_short_name());
    root.put("tsbk", tsbk);

    std::stringstream tsbk_str;
    boost::property_tree::write_json(tsbk_str, root);

    // std::cout << stats_str;
    TSBK_Socket::send_msg(tsbk_str.str());
  }

  int TSBK_Socket::send_msg(std::string val) {
    char* c = const_cast<char*>(val.c_str());

    sendto(m_sockfd, c, strlen(c), 0,
         (struct sockaddr*)&m_servaddr, m_servlen);
    return 0;
  }

void TSBK_Socket::send_mbt(boost::dynamic_bitset<> &tsbk, System *system) {

    if (m_server.empty())
      return;

    boost::property_tree::ptree root;

    root.put("version", "V1");
    root.put("type", "mbt");
    root.put("sysName", system->get_short_name());
    root.put("tsbk", tsbk);

    std::stringstream tsbk_str;
    boost::property_tree::write_json(tsbk_str, root);

    // std::cout << stats_str;
    TSBK_Socket::send_msg(tsbk_str.str());
  }

  int TSBK_Socket::send_msg(std::string val) {
    char* c = const_cast<char*>(val.c_str());

    sendto(m_sockfd, c, strlen(c), 0,
         (struct sockaddr*)&m_servaddr, m_servlen);
    return 0;
  }
