#ifndef UPLOADER_H
#define UPLOADER_H

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ssl.hpp>
#include <pthread.h>
#include <sstream>

#include "config.h"
#include "call.h"
#include "url_parser/url_parser.h"

struct call_data_t{
  long thread_id;
  long talkgroup;
	double freq;
	long start_time;
  bool encrypted;
  bool emergency;
  char filename[160];
  char converted[160];
  std::string upload_server;
  std::string server;
  std::string scheme;
  std::string hostname;
  std::string port;
  std::string path;
  int tdma;
  long source_count;
 Call_Source source_list[50];
};

struct server_data_t{
  std::string upload_server;
  std::string server;
  std::string scheme;
  std::string hostname;
  std::string port;
  std::string path;
};

int upload(struct call_data_t *call);
void *convert_upload_call(void *thread_arg);
void send_call(Call *call, Config config);


#endif
