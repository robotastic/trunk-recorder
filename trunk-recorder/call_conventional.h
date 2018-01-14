#ifndef CALL_CONVENTIONAL_H
#define CALL_CONVENTIONAL_H
class Config;
class System;
class Recorder;
class stat_socket;

#include <string>
#include "call.h"


class Call_conventional : public Call {
public:

								Call_conventional( long t, double f, System *s, Config c, stat_socket * stat_socket);
								~Call_conventional();

								virtual bool is_conventional() { return true;}
								void restart_call();
								void set_recorder(Recorder *r, std::string device);
								char * get_filename();
								void recording_started();
private:								
								stat_socket * d_stat_socket;								
};

#endif
