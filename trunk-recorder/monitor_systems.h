#ifndef MONITOR_SYSTEMS_H
#define MONITOR_SYSTEMS_H
#include <stdlib.h>
#include <signal.h>

#include <gnuradio/top_block.h>
#include "./global_structs.h"
#include "call.h"
#include "source.h"
#include "config.h"
#include "systems/p25_trunking.h"
#include "systems/p25_parser.h"
#include "systems/smartnet_trunking.h"
#include "systems/smartnet_parser.h"
#include "systems/system.h"

int monitor_messages(Config &config, gr::top_block_sptr &tb, std::vector<Source *> &sources, std::vector<System *> &systems, std::vector<Call *> &calls);
#endif