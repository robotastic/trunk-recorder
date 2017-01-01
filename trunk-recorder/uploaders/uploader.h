#ifndef UPLOADER_H
#define UPLOADER_H

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/detail/socket_option.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/regex.hpp>
#include <boost/log/trivial.hpp>
#include <pthread.h>
#include <sstream>

#include "../config.h"
#include "../systems/system.h"

class Call;

#include "../call.h"


struct server_data_t {
        std::string upload_server;
        std::string server;
        std::string scheme;
        std::string hostname;
        std::string port;
        std::string path;
};

void add_post_field(std::ostringstream& post_stream, std::string name, std::string value, std::string boundary);
int http_upload(struct server_data_t *server_info,   boost::asio::streambuf& request_);
int https_upload(struct server_data_t *server_info, boost::asio::streambuf& request_);



#endif
