#ifndef CALL_CONVENTIONAL_H
#define CALL_CONVENTIONAL_H
#include "global_structs.h"
class System;
class Recorder;

#include "call.h"
#include <string>

class Call_conventional : public Call {
public:
  Call_conventional(long t, double f, System *s, Config c);
  ~Call_conventional();
  time_t get_start_time();
  virtual bool is_conventional() { return true; }
  void restart_call();
  void set_recorder(Recorder *r);
  char *get_filename();
  void recording_started();
};

#endif
