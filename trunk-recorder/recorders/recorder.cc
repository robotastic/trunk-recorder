#include "recorder.h"
#include "../source.h"
#include <boost/algorithm/string.hpp>

Recorder::Recorder(Recorder_Type type, Config c) {
  this->type = type;
  config = c;
}

int Recorder::get_instance_id() {
    return this->config.instance_id;
}

boost::property_tree::ptree Recorder::get_stats() {
  boost::property_tree::ptree node;
  node.put("id", boost::lexical_cast<std::string>(get_source()->get_num()) + "_" + boost::lexical_cast<std::string>(get_num()));
  node.put("instance_id", get_instance_id);
  node.put("type", get_type_string());
  node.put("src_num", get_source()->get_num());
  node.put("rec_num", get_num());
  node.put("count", recording_count);
  node.put("duration", recording_duration);
  node.put("state", get_state());
  return node;
}

std::string Recorder::get_type_string() {
  switch(type) {
    case DEBUG:
      return "Debug";
    case SIGMF:
      return "SIGMF";
    case ANALOGC:
      return "AnalogC";
    case ANALOG:
      return "Analog";
    case P25:
      return "P25";
    case P25C:
      return "P25C";
    case DMR:
      return "DMR";
    default:
      return "Unknown";
  }
}