#include "../plugin_api.h"
#include "../../systems/system.h"
#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS

struct Unit_Script_System_Script {
  std::string script;
  std::string short_name;
};

class Unit_Script : public Plugin_Api {
    std::vector<Unit_Script_System_Script> system_scripts;
std::map<long, long> unit_affiliations;
public:
  std::string get_system_script(std::string short_name) {
    for (std::vector<Unit_Script_System_Script>::iterator it = system_scripts.begin(); it != system_scripts.end(); ++it) {
      Unit_Script_System_Script system_script = *it;
      if (system_script.short_name == short_name){
        return system_script.script;
      }
    }
    return "";
  }

int unit_registration(System *sys, long source_id) {
    unit_affiliations[source_id] = 0;
  std::string system_script = get_system_script(sys->get_short_name());
  if ((system_script != "") && (source_id != 0)) {
    char shell_command[200];
    sprintf(shell_command, "%s %s %li on &", system_script.c_str(), sys->get_short_name().c_str(), source_id);
    int rc =  system(shell_command);
    return 0;
  }
    return 1;
}

int unit_deregistration(System *sys, long source_id) { 
    unit_affiliations[source_id] = -1;
  std::string system_script = get_system_script(sys->get_short_name());
  if ((system_script != "") && (source_id != 0)) {
    char shell_command[200];
    sprintf(shell_command, "%s %s %li off &", system_script.c_str(), sys->get_short_name().c_str(), source_id);
    int rc =  system(shell_command);
    return 0;
  }
    return 1;
}  
int unit_acknowledge_response(System *sys, long source_id) { 
  std::string system_script = get_system_script(sys->get_short_name());
  if ((system_script != "") && (source_id != 0)) {
    char shell_command[200];
    sprintf(shell_command, "%s %s %li ackresp &", system_script.c_str(), sys->get_short_name().c_str(), source_id);
    int rc =  system(shell_command);
    return 0;
  }
    return 1;
}



int unit_group_affiliation(System *sys, long source_id, long talkgroup_num) {
    unit_affiliations[source_id] = talkgroup_num;
    std::string system_script = get_system_script(sys->get_short_name());
  if ((system_script != "") && (source_id != 0)) {
    char shell_command[200];
    sprintf(shell_command, "%s %s %li join %li &", system_script.c_str(), sys->get_short_name().c_str(), source_id, talkgroup_num);
    int rc =  system(shell_command);
    return 0;
  }
    return 1;
}


  int parse_config(boost::property_tree::ptree &cfg) {

    BOOST_FOREACH (boost::property_tree::ptree::value_type &node, cfg.get_child("systems")) {
      Unit_Script_System_Script system_script;
      system_script.script = node.second.get<std::string>("script", "");
      system_script.short_name = node.second.get<std::string>("shortName", "");
      BOOST_LOG_TRIVIAL(info) << "Unit Script for: " << system_script.short_name;
      this->system_scripts.push_back(system_script);
    }

    return 0;
  }

  static boost::shared_ptr<Unit_Script> create() {
    return boost::shared_ptr<Unit_Script>(
        new Unit_Script());
  }
};

BOOST_DLL_ALIAS(
    Unit_Script::create, // <-- this function is exported with...
    create_plugin             // <-- ...this alias name
)
