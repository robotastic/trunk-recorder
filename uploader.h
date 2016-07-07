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


#include "call.h"

struct call_data{
  long thread_id;
  long talkgroup;
	double freq;
	long start_time;
  bool encrypted;
  bool emergency;
  char filename[160];
  char converted[160];
  int tdma;
  long src_count;
  long src_list[50];
};
int upload(struct call_data *call);
void *convert_upload_call(void *thread_arg);
void send_call(Call *call);


#endif
