#ifndef CALL_UPLOADER_H
#define CALL_UPLOADER_H

#include <vector>

class Call;


class System;

#include "../formatter.h"
#include "../call.h"
#include "../systems/system.h"
#include "../config.h"

void send_call(Call *call, System *sys, Config config);


#endif
