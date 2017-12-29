#ifndef CALL_CONVENTIONAL_H
#define CALL_CONVENTIONAL_H
class Config;
class System;
class Recorder;


#include <string>
#include "call.h"


class Call_conventional : public Call {
public:

								Call_conventional( long t, double f, System *s, Config c);
								~Call_conventional();

								virtual bool is_conventional() { return true;}
								void restart_call();
								void set_recorder(Recorder *r, std::string device);
								char * get_filename();
};

#endif
