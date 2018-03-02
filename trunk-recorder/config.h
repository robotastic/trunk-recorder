#ifndef CONFIG_H
#define CONFIG_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <stdlib.h>
#include <string>

/*
#include "systems/system.h"
#include "source.h"
*/

struct Config {
        std::string upload_script;
        std::string upload_server;
        std::string status_server;
        std::string instance_key;
        std::string instance_id;
        std::string capture_dir;
        int call_timeout;
        bool log_file;
        int control_message_warn_rate;
        int control_retune_limit;
};

//Config load_config(std::string config_file, std::vector<Source *> &sources, std::vector<System *> &systems);


#endif
