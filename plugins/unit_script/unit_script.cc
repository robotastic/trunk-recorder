#include "../../trunk-recorder/plugin_manager/plugin_api.h"
#include "../../trunk-recorder/systems/system.h"
#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS
#include <boost/foreach.hpp>


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
    snprintf(shell_command, 200, "%s %s %li on &", system_script.c_str(), sys->get_short_name().c_str(), source_id);
    int rc __attribute__((unused)) =  system(shell_command);
    return 0;
  }
    return 1;
}

int unit_deregistration(System *sys, long source_id) { 
    unit_affiliations[source_id] = -1;
  std::string system_script = get_system_script(sys->get_short_name());
  if ((system_script != "") && (source_id != 0)) {
    char shell_command[200];
    snprintf(shell_command, 200, "%s %s %li off &", system_script.c_str(), sys->get_short_name().c_str(), source_id);
    int rc __attribute__((unused)) =  system(shell_command);
    return 0;
  }
    return 1;
}  
int unit_acknowledge_response(System *sys, long source_id) { 
  std::string system_script = get_system_script(sys->get_short_name());
  if ((system_script != "") && (source_id != 0)) {
    char shell_command[200];
    snprintf(shell_command,200, "%s %s %li ackresp &", system_script.c_str(), sys->get_short_name().c_str(), source_id);
    int rc __attribute__((unused)) =  system(shell_command);
    return 0;
  }
    return 1;
}

int unit_group_affiliation(System *sys, long source_id, long talkgroup_num) {
    unit_affiliations[source_id] = talkgroup_num;
    std::string system_script = get_system_script(sys->get_short_name());
  if ((system_script != "") && (source_id != 0)) {
    char shell_command[200];
    std::vector<unsigned long> talkgroup_patches = sys->get_talkgroup_patch(talkgroup_num);
    std::string patch_string;
    bool first = true;
    BOOST_FOREACH (auto& TGID, talkgroup_patches) {
      if (!first) { patch_string += ","; }
      first = false;
      patch_string += std::to_string(TGID);
    }
    snprintf(shell_command, 200, "%s %s %li join %li %s &", system_script.c_str(), sys->get_short_name().c_str(), source_id, talkgroup_num, patch_string.c_str());
    int rc __attribute__((unused)) =  system(shell_command);
    return 0;
  }
    return 1;
}

int unit_data_grant(System *sys, long source_id) {
    std::string system_script = get_system_script(sys->get_short_name());
  if ((system_script != "") && (source_id != 0)) {
    char shell_command[200];
    snprintf(shell_command, 200, "%s %s %li data &", system_script.c_str(), sys->get_short_name().c_str(), source_id);
    int rc __attribute__((unused)) =  system(shell_command);
    return 0;
  }
    return 1;
}

int unit_answer_request(System *sys, long source_id, long talkgroup) {
    std::string system_script = get_system_script(sys->get_short_name());
  if ((system_script != "") && (source_id != 0)) {
    char shell_command[200];
    snprintf(shell_command, 200, "%s %s %li ans_req %li &", system_script.c_str(), sys->get_short_name().c_str(), source_id, talkgroup);
    int rc __attribute__((unused)) =  system(shell_command);
    return 0;
  }
    return 1;
}

int unit_location(System *sys, long source_id, long talkgroup_num) {
    unit_affiliations[source_id] = talkgroup_num;
    std::string system_script = get_system_script(sys->get_short_name());
  if ((system_script != "") && (source_id != 0)) {
    char shell_command[200];
    std::vector<unsigned long> talkgroup_patches = sys->get_talkgroup_patch(talkgroup_num);
    std::string patch_string;
    bool first = true;
    BOOST_FOREACH (auto& TGID, talkgroup_patches) {
      if (!first) { patch_string += ","; }
      first = false;
      patch_string += std::to_string(TGID);
    }
    snprintf(shell_command, 200, "%s %s %li location %li %s &", system_script.c_str(), sys->get_short_name().c_str(), source_id, talkgroup_num, patch_string.c_str());
    int rc __attribute__((unused)) =  system(shell_command);
    return 0;
  }
    return 1;
}

int call_start(Call *call) {
    long talkgroup_num = call->get_talkgroup();
    long source_id = call->get_current_source_id();
    std::string short_name = call->get_short_name();
    std::string system_script = get_system_script(short_name);
  if ((system_script != "") && (source_id != 0)) {
    char shell_command[200];
    std::vector<unsigned long> talkgroup_patches = call->get_system()->get_talkgroup_patch(talkgroup_num);
    std::string patch_string;
    bool first = true;
    BOOST_FOREACH (auto& TGID, talkgroup_patches) {
      if (!first) { patch_string += ","; }
      first = false;
      patch_string += std::to_string(TGID);
    }
    snprintf(shell_command, 200, "%s %s %li call %li %s &", system_script.c_str(), short_name.c_str(), source_id, talkgroup_num, patch_string.c_str());
    int rc __attribute__((unused)) =  system(shell_command);
    return 0;
  }
    return 1;
}

  int parse_config(json config_data) {

    for (json element : config_data["systems"]) {
      Unit_Script_System_Script system_script;
      system_script.script = element.value("unitScript", "");
      system_script.short_name = element.value("shortName", "");
      if (system_script.script != "") {
        BOOST_LOG_TRIVIAL(info) << "\t- [" << system_script.short_name << "]: " << system_script.script ;
        this->system_scripts.push_back(system_script);
      }
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
