#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <string>

struct Config {
        std::string upload_script;
        std::string upload_server;
        std::string capture_dir;
        int call_timeout;
        bool log_file;
};

#endif
