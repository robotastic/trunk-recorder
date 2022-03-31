
#ifndef RXSTATUS_H
#define RXSTATUS_H
#include <time.h>
struct Rx_Status{
  double total_len;
  long error_count;
  long spike_count;
  time_t last_update;
};
#endif
