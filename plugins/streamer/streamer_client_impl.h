#ifndef STREAMER_CLIENT_IMPL_H
#define STREAMER_CLIENT_IMPL_H

#include <stdbool.h>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include "../trunk-recorder/plugin_manager/plugin_common.h"
#include "StreamerClient.h"

typedef struct streamer_client_plugin_t streamer_client_plugin_t;

struct streamer_client_plugin_t {
    bool enable_audio_streaming;
    std::string server_addr;
    StreamerClient client;
};

#endif // STREAMER_CLIENT_IMPL_H