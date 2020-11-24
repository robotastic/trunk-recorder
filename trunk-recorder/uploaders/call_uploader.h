#ifndef CALL_UPLOADER_H
#define CALL_UPLOADER_H

#include <vector>

class Call;

class System;

#include "../call.h"
#include "../config.h"
#include "../formatter.h"
#include "../systems/system.h"

void send_call(Call *call, System *sys, Config config);
void send_transmissions(Call *call, System *sys, Config config);
#endif
