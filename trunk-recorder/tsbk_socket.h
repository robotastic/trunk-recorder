#if !defined(__TSBK_Socket_H__)
#define __TSBK_Socket_H__

#include <bits/stdc++.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <boost/property_tree/json_parser.hpp>
#include <boost/dynamic_bitset.hpp>
#include "systems/system.h"

class TSBK_Socket {

    public:
    TSBK_Socket(std::string server_addr, int port);
    void send_tsbk(boost::dynamic_bitset<> &tsbk, System *system);
    int send_msg(std::string val);

    private:
    std::string m_server;
    int m_port;
    int m_sockfd;
    struct sockaddr_in m_servaddr;
    int m_servlen;

};

inline TSBK_Socket *tsbk_socket;

#endif