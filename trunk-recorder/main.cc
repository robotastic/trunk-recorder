#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/foreach.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/tokenizer.hpp>

#include <cassert>
#include <exception>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "recorder_globals.h"
#include "source.h"

#include "recorders/analog_recorder.h"
#include "recorders/p25_recorder.h"
#include "recorders/recorder.h"

#include "call.h"
#include "call_conventional.h"
#include "systems/p25_parser.h"
#include "systems/p25_trunking.h"
#include "systems/parser.h"
#include "systems/smartnet_parser.h"
#include "systems/smartnet_trunking.h"
#include "systems/system.h"

#include <osmosdr/source.h>

#include "formatter.h"
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/gr_complex.h>
#include <gnuradio/message.h>
#include <gnuradio/msg_queue.h>
#include <gnuradio/top_block.h>
#include <gnuradio/uhd/usrp_source.h>

#include "plugins/plugin-manager.h"

using namespace std;
namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
int Recorder::rec_counter = 0;

std::vector<Source *> sources;
std::vector<System *> systems;
std::map<long, long> unit_affiliations;

std::vector<Call *> calls;

gr::top_block_sptr tb;
gr::msg_queue::sptr msg_queue;

volatile sig_atomic_t exit_flag = 0;
SmartnetParser *smartnet_parser;
P25Parser *p25_parser;
Config config;
string default_mode;

void exit_interupt(int sig) { // can be called asynchronously
  exit_flag = 1;              // set flag
}

unsigned GCD(unsigned u, unsigned v) {
  while (v != 0) {
    unsigned r = u % v;
    u = v;
    v = r;
  }
  return u;
}

std::vector<float> design_filter(double interpolation, double deci) {
  float beta = 5.0;
  float trans_width = 0.5 - 0.4;
  float mid_transition_band = 0.5 - trans_width / 2;

  std::vector<float> result = gr::filter::firdes::low_pass(
      interpolation,
      1,
      mid_transition_band / interpolation,
      trans_width / interpolation,
      gr::filter::firdes::WIN_KAISER,
      beta);

  return result;
}

void set_logging_level(std::string log_level) {
  boost::log::trivial::severity_level sev_level = boost::log::trivial::info;

  if (log_level == "trace")
    sev_level = boost::log::trivial::trace;
  else if (log_level == "debug")
    sev_level = boost::log::trivial::debug;
  else if (log_level == "info")
    sev_level = boost::log::trivial::info;
  else if (log_level == "warning")
    sev_level = boost::log::trivial::warning;
  else if (log_level == "error")
    sev_level = boost::log::trivial::error;
  else if (log_level == "fatal")
    sev_level = boost::log::trivial::fatal;
  else {
    BOOST_LOG_TRIVIAL(error) << "set_logging_level: Unknown logging level: " << log_level;
    return;
  }

  logging::core::get()->set_filter(
      logging::trivial::severity >= sev_level

  );

  boost::log::core::get()->set_filter(
      boost::log::trivial::severity >= sev_level);
}

/**
 * Method name: load_config()
 * Description: <#description#>
 * Parameters: <#parameters#>
 */
bool load_config(string config_file) {
  string system_modulation;
  int sys_count = 0;
  int source_count = 0;

  try {
    // const std::string json_filename = "config.json";

    boost::property_tree::ptree pt;
    boost::property_tree::read_json(config_file, pt);
    double config_ver = pt.get<double>("ver", 0);
    if (config_ver < 2) {
      BOOST_LOG_TRIVIAL(info) << "The formatting for config files has changed.";
      BOOST_LOG_TRIVIAL(info) << "Modulation type, Squelch and audio levels are now set in each System instead of under a Source.";
      BOOST_LOG_TRIVIAL(info) << "See sample config files in the /example folder and look at readme.md for more details.";
      BOOST_LOG_TRIVIAL(info) << "After you have made these updates, make sure you add \"ver\": 2, to the top.\n\n";
      return false;
    }

    BOOST_LOG_TRIVIAL(info) << "\n-------------------------------------\n     Trunk Recorder\n-------------------------------------\n";
    BOOST_LOG_TRIVIAL(info) << "\n-------------------------------------\nSYSTEMS\n-------------------------------------\n";

    std::string frequencyFormatString = pt.get<std::string>("frequencyFormat", "exp");

    if (boost::iequals(frequencyFormatString, "mhz")) {
      frequencyFormat = 1;
    } else if (boost::iequals(frequencyFormatString, "hz")) {
      frequencyFormat = 2;
    } else {
      frequencyFormat = 0;
    }

    config.debug_recorder = pt.get<bool>("debugRecorder", 0);
    config.debug_recorder_address = pt.get<std::string>("debugRecorderAddress", "127.0.0.1");
    config.debug_recorder_port = pt.get<int>("debugRecorderPort", 1234);

    BOOST_FOREACH (boost::property_tree::ptree::value_type &node,
                   pt.get_child("systems")) {
      // each system should have a unique index value;
      System *system = new System(sys_count++);

      std::stringstream default_script;
      unsigned long sys_id;
      unsigned long wacn;
      unsigned long nac;
      default_script << "sys_" << sys_count;

      BOOST_LOG_TRIVIAL(info) << "\n\nSystem Number: " << sys_count << "\n-------------------------------------\n";
      system->set_short_name(node.second.get<std::string>("shortName", default_script.str()));
      BOOST_LOG_TRIVIAL(info) << "Short Name: " << system->get_short_name();

      system->set_system_type(node.second.get<std::string>("type"));
      BOOST_LOG_TRIVIAL(info) << "System Type: " << system->get_system_type();

      if (system->get_system_type() == "conventional") {
        BOOST_LOG_TRIVIAL(info) << "Conventional Channels: ";
        BOOST_FOREACH (boost::property_tree::ptree::value_type &sub_node, node.second.get_child("channels")) {
          double channel = sub_node.second.get<double>("", 0);

          BOOST_LOG_TRIVIAL(info) << "  " << FormatFreq(channel);
          system->add_channel(channel);
        }

        BOOST_LOG_TRIVIAL(info) << "Alpha Tags: ";
        if (node.second.count("alphatags") != 0) {
          int alphaIndex = 1;
          BOOST_FOREACH (boost::property_tree::ptree::value_type &sub_node, node.second.get_child("alphatags")) {
            std::string alphaTag = sub_node.second.get<std::string>("", "");
            BOOST_LOG_TRIVIAL(info) << "  " << alphaTag;
            system->talkgroups->add(alphaIndex, alphaTag);
            alphaIndex++;
          }
        }

      } else if (system->get_system_type() == "conventionalP25") {
        BOOST_LOG_TRIVIAL(info) << "Conventional Channels: ";
        BOOST_FOREACH (boost::property_tree::ptree::value_type &sub_node, node.second.get_child("channels")) {
          double channel = sub_node.second.get<double>("", 0);

          BOOST_LOG_TRIVIAL(info) << "  " << FormatFreq(channel);
          system->add_channel(channel);
        }

        BOOST_LOG_TRIVIAL(info) << "Alpha Tags: ";
        if (node.second.count("alphatags") != 0) {
          int alphaIndex = 1;
          BOOST_FOREACH (boost::property_tree::ptree::value_type &sub_node, node.second.get_child("alphatags")) {
            std::string alphaTag = sub_node.second.get<std::string>("", "");
            BOOST_LOG_TRIVIAL(info) << "  " << alphaTag;
            system->talkgroups->add(alphaIndex, alphaTag);
            alphaIndex++;
          }
        }

        system->set_delaycreateoutput(node.second.get<bool>("delayCreateOutput", false));
        BOOST_LOG_TRIVIAL(info) << "delayCreateOutput: " << system->get_delaycreateoutput();

      } else if ((system->get_system_type() == "smartnet") || (system->get_system_type() == "p25")) {
        BOOST_LOG_TRIVIAL(info) << "Control Channels: ";
        BOOST_FOREACH (boost::property_tree::ptree::value_type &sub_node, node.second.get_child("control_channels")) {
          double control_channel = sub_node.second.get<double>("", 0);

          BOOST_LOG_TRIVIAL(info) << "  " << FormatFreq(control_channel);
          system->add_control_channel(control_channel);
        }
      } else {
        BOOST_LOG_TRIVIAL(error) << "System Type in config.json not recognized";
        exit(1);
      }

      bool qpsk_mod = true;
      double digital_levels = node.second.get<double>("digitalLevels", 1.0);
      double analog_levels = node.second.get<double>("analogLevels", 8.0);
      double squelch_db = node.second.get<double>("squelch", -160);
      int max_dev = node.second.get<int>("maxDev", 4000);
      double filter_width = node.second.get<double>("filterWidth", 1.0);
      bool conversation_mode = node.second.get<bool>("conversationMode", true);
      boost::optional<std::string> mod_exists = node.second.get_optional<std::string>("modulation");

      if (mod_exists) {
        system_modulation = node.second.get<std::string>("modulation");

        if (boost::iequals(system_modulation, "qpsk")) {
          qpsk_mod = true;
          BOOST_LOG_TRIVIAL(info) << "Modulation: qpsk";
        } else if (boost::iequals(system_modulation, "fsk4")) {
          qpsk_mod = false;
          BOOST_LOG_TRIVIAL(info) << "Modulation: fsk4";
        } else {
          qpsk_mod = true;
          BOOST_LOG_TRIVIAL(error) << "! System Modulation Not Specified, could be fsk4 or qpsk, assuming qpsk";
        }
      } else {
        qpsk_mod = true;
      }

      system->set_squelch_db(squelch_db);
      system->set_analog_levels(analog_levels);
      system->set_digital_levels(digital_levels);
      system->set_qpsk_mod(qpsk_mod);
      system->set_max_dev(max_dev);
      system->set_filter_width(filter_width);
      system->set_conversation_mode(conversation_mode);
      BOOST_LOG_TRIVIAL(info) << "Conversation Mode: " << conversation_mode;
      BOOST_LOG_TRIVIAL(info) << "Analog Recorder Maximum Deviation: " << node.second.get<int>("maxDev", 4000);
      BOOST_LOG_TRIVIAL(info) << "Filter Width: " << filter_width;
      BOOST_LOG_TRIVIAL(info) << "Squelch: " << node.second.get<double>("squelch", -160);
      system->set_api_key(node.second.get<std::string>("apiKey", ""));
      BOOST_LOG_TRIVIAL(info) << "API Key: " << system->get_api_key();
      system->set_bcfy_api_key(node.second.get<std::string>("broadcastifyApiKey", ""));
      BOOST_LOG_TRIVIAL(info) << "Broadcastify API Key: " << system->get_bcfy_api_key();
      system->set_bcfy_system_id(node.second.get<int>("broadcastifySystemId", 0));
      BOOST_LOG_TRIVIAL(info) << "Broadcastify Calls System ID: " << system->get_bcfy_system_id();
      system->set_upload_script(node.second.get<std::string>("uploadScript", ""));
      BOOST_LOG_TRIVIAL(info) << "Upload Script: " << config.upload_script;
      system->set_call_log(node.second.get<bool>("callLog", true));
      BOOST_LOG_TRIVIAL(info) << "Call Log: " << system->get_call_log();
      system->set_audio_archive(node.second.get<bool>("audioArchive", true));
      BOOST_LOG_TRIVIAL(info) << "Audio Archive: " << system->get_audio_archive();
      system->set_talkgroups_file(node.second.get<std::string>("talkgroupsFile", ""));
      BOOST_LOG_TRIVIAL(info) << "Talkgroups File: " << system->get_talkgroups_file();
      system->set_unit_tags_file(node.second.get<std::string>("unitTagsFile", ""));
      BOOST_LOG_TRIVIAL(info) << "Unit Tags File: " << system->get_unit_tags_file();
      system->set_record_unknown(node.second.get<bool>("recordUnknown", true));
      BOOST_LOG_TRIVIAL(info) << "Record Unknown Talkgroups: " << system->get_record_unknown();
      system->set_mdc_enabled(node.second.get<bool>("decodeMDC", false));
      BOOST_LOG_TRIVIAL(info) << "Decode MDC: " << system->get_mdc_enabled();
      system->set_fsync_enabled(node.second.get<bool>("decodeFSync", false));
      BOOST_LOG_TRIVIAL(info) << "Decode FSync: " << system->get_fsync_enabled();
      system->set_star_enabled(node.second.get<bool>("decodeStar", false));
      BOOST_LOG_TRIVIAL(info) << "Decode Star: " << system->get_star_enabled();
      system->set_tps_enabled(node.second.get<bool>("decodeTPS", false));
      BOOST_LOG_TRIVIAL(info) << "Decode TPS: " << system->get_tps_enabled();
      std::string talkgroup_display_format_string = node.second.get<std::string>("talkgroupDisplayFormat", "Id");
      if (boost::iequals(talkgroup_display_format_string, "id_tag")) {
        system->set_talkgroup_display_format(System::talkGroupDisplayFormat_id_tag);
      } else if (boost::iequals(talkgroup_display_format_string, "tag_id")) {
        system->set_talkgroup_display_format(System::talkGroupDisplayFormat_tag_id);
      } else {
        system->set_talkgroup_display_format(System::talkGroupDisplayFormat_id);
      }
      BOOST_LOG_TRIVIAL(info) << "Talkgroup Display Format: " << talkgroup_display_format_string;

      sys_id = node.second.get<unsigned long>("sysId", 0);
      nac = node.second.get<unsigned long>("nac", 0);
      wacn = node.second.get<unsigned long>("wacn", 0);
      system->set_xor_mask(sys_id, wacn, nac);
      system->set_bandplan(node.second.get<std::string>("bandplan", "800_standard"));
      system->set_bandfreq(800); // Default to 800

      if (boost::starts_with(system->get_bandplan(), "400")) {
        system->set_bandfreq(400);
      }
      system->set_bandplan_base(node.second.get<double>("bandplanBase", 0.0));
      system->set_bandplan_high(node.second.get<double>("bandplanHigh", 0.0));
      system->set_bandplan_spacing(node.second.get<double>("bandplanSpacing", 0.0));
      system->set_bandplan_offset(node.second.get<int>("bandplanOffset", 0));

      if (system->get_system_type() == "smartnet") {
        BOOST_LOG_TRIVIAL(info) << "Smartnet bandplan: " << system->get_bandplan();
        BOOST_LOG_TRIVIAL(info) << "Smartnet band: " << system->get_bandfreq();

        if (system->get_bandplan_base() || system->get_bandplan_spacing() || system->get_bandplan_offset()) {
          BOOST_LOG_TRIVIAL(info) << "Smartnet bandplan base freq: " << system->get_bandplan_base();
          BOOST_LOG_TRIVIAL(info) << "Smartnet bandplan high freq: " << system->get_bandplan_high();
          BOOST_LOG_TRIVIAL(info) << "Smartnet bandplan spacing: " << system->get_bandplan_spacing();
          BOOST_LOG_TRIVIAL(info) << "Smartnet bandplan offset: " << system->get_bandplan_offset();
        }
      }

      system->set_hideEncrypted(node.second.get<bool>("hideEncrypted", system->get_hideEncrypted()));
      BOOST_LOG_TRIVIAL(info) << "Hide Encrypted Talkgroups: " << system->get_hideEncrypted();
      system->set_hideUnknown(node.second.get<bool>("hideUnknownTalkgroups", system->get_hideUnknown()));
      BOOST_LOG_TRIVIAL(info) << "Hide Unknown Talkgroups: " << system->get_hideUnknown();
      system->set_min_duration(node.second.get<double>("minDuration", 0));
      BOOST_LOG_TRIVIAL(info) << "Minimum Call Duration (in seconds): " << system->get_min_duration();

      systems.push_back(system);
      BOOST_LOG_TRIVIAL(info);
    }

    BOOST_LOG_TRIVIAL(info) << "\n\n-------------------------------------\nSOURCES\n-------------------------------------\n";
    BOOST_FOREACH (boost::property_tree::ptree::value_type &node,
                   pt.get_child("sources")) {

      bool gain_set = false;
      int silence_frames = node.second.get<int>("silenceFrames", 0);
      double center = node.second.get<double>("center", 0);
      double rate = node.second.get<double>("rate", 0);
      double error = node.second.get<double>("error", 0);
      double ppm = node.second.get<double>("ppm", 0);
      bool agc = node.second.get<bool>("agc", false);
      int gain = node.second.get<double>("gain", 0);
      int if_gain = node.second.get<double>("ifGain", 0);
      int bb_gain = node.second.get<double>("bbGain", 0);
      int mix_gain = node.second.get<double>("mixGain", 0);
      int lna_gain = node.second.get<double>("lnaGain", 0);
      int pga_gain = node.second.get<double>("pgaGain", 0);
      int tia_gain = node.second.get<double>("tiaGain", 0);
      int vga1_gain = node.second.get<double>("vga1Gain", 0);
      int vga2_gain = node.second.get<double>("vga2Gain", 0);

      std::string antenna = node.second.get<string>("antenna", "");
      int digital_recorders = node.second.get<int>("digitalRecorders", 0);
      int sigmf_recorders = node.second.get<int>("sigmfRecorders", 0);
      int analog_recorders = node.second.get<int>("analogRecorders", 0);

      std::string driver = node.second.get<std::string>("driver", "");

      if ((driver != "osmosdr") && (driver != "usrp")) {
        BOOST_LOG_TRIVIAL(error) << "Driver specified in config.json not recognized, needs to be osmosdr or usrp";
      }

      std::string device = node.second.get<std::string>("device", "");
      BOOST_LOG_TRIVIAL(info) << "Driver: " << node.second.get<std::string>("driver", "");
      BOOST_LOG_TRIVIAL(info) << "Center: " << FormatFreq(node.second.get<double>("center", 0));
      BOOST_LOG_TRIVIAL(info) << "Rate: " << FormatSamplingRate(node.second.get<double>("rate", 0));
      BOOST_LOG_TRIVIAL(info) << "Error: " << node.second.get<double>("error", 0);
      BOOST_LOG_TRIVIAL(info) << "PPM Error: " << node.second.get<double>("ppm", 0);
      BOOST_LOG_TRIVIAL(info) << "Auto gain control: " << node.second.get<bool>("agc", false);
      BOOST_LOG_TRIVIAL(info) << "Gain: " << node.second.get<double>("gain", 0);
      BOOST_LOG_TRIVIAL(info) << "IF Gain: " << node.second.get<double>("ifGain", 0);
      BOOST_LOG_TRIVIAL(info) << "BB Gain: " << node.second.get<double>("bbGain", 0);
      BOOST_LOG_TRIVIAL(info) << "LNA Gain: " << node.second.get<double>("lnaGain", 0);
      BOOST_LOG_TRIVIAL(info) << "PGA Gain: " << node.second.get<double>("pgaGain", 0);
      BOOST_LOG_TRIVIAL(info) << "TIA Gain: " << node.second.get<double>("tiaGain", 0);
      BOOST_LOG_TRIVIAL(info) << "MIX Gain: " << node.second.get<double>("mixGain", 0);
      BOOST_LOG_TRIVIAL(info) << "VGA1 Gain: " << node.second.get<double>("vga1Gain", 0);
      BOOST_LOG_TRIVIAL(info) << "VGA2 Gain: " << node.second.get<double>("vga2Gain", 0);
      BOOST_LOG_TRIVIAL(info) << "Idle Silence: " << node.second.get<bool>("idleSilence", 0);
      BOOST_LOG_TRIVIAL(info) << "Digital Recorders: " << node.second.get<int>("digitalRecorders", 0);
      BOOST_LOG_TRIVIAL(info) << "Debug Recorder: " << node.second.get<bool>("debugRecorder", 0);
      BOOST_LOG_TRIVIAL(info) << "SigMF Recorders: " << node.second.get<int>("sigmfRecorders", 0);
      BOOST_LOG_TRIVIAL(info) << "Analog Recorders: " << node.second.get<int>("analogRecorders", 0);

      if ((ppm != 0) && (error != 0)) {
        BOOST_LOG_TRIVIAL(info) << "Both PPM and Error should not be set at the same time. Setting Error to 0.";
        error = 0;
      }

      Source *source = new Source(center, rate, error, driver, device, &config);
      BOOST_LOG_TRIVIAL(info) << "Max Frequency: " << FormatFreq(source->get_max_hz());
      BOOST_LOG_TRIVIAL(info) << "Min Frequency: " << FormatFreq(source->get_min_hz());

      if (if_gain != 0) {
        gain_set = true;
        source->set_if_gain(if_gain);
      }

      if (bb_gain != 0) {
        gain_set = true;
        source->set_bb_gain(bb_gain);
      }

      if (mix_gain != 0) {
        gain_set = true;
        source->set_mix_gain(mix_gain);
      }

      if (lna_gain != 0) {
        gain_set = true;
        source->set_lna_gain(lna_gain);
      }

      if (tia_gain != 0) {
        gain_set = true;
        source->set_tia_gain(tia_gain);
      }

      if (pga_gain != 0) {
        gain_set = true;
        source->set_pga_gain(pga_gain);
      }

      if (vga1_gain != 0) {
        gain_set = true;
        source->set_vga1_gain(vga1_gain);
      }

      if (vga2_gain != 0) {
        gain_set = true;
        source->set_vga2_gain(vga2_gain);
      }

      if (gain != 0) {
        gain_set = true;
        source->set_gain(gain);
      }

      if (!gain_set) {
        BOOST_LOG_TRIVIAL(error) << "! No Gain was specified! Things will probably not work";
      }

      source->set_gain_mode(agc);
      source->set_antenna(antenna);
      source->set_silence_frames(silence_frames);

      if (ppm != 0) {
        source->set_freq_corr(ppm);
      }
      source->create_digital_recorders(tb, digital_recorders);
      source->create_analog_recorders(tb, analog_recorders);
      source->create_sigmf_recorders(tb, sigmf_recorders);
      if (config.debug_recorder) {
        source->create_debug_recorder(tb, source_count);
      }
      sources.push_back(source);
      source_count++;
      BOOST_LOG_TRIVIAL(info) << "\n-------------------------------------\n\n";
    }

    BOOST_LOG_TRIVIAL(info) << "\n\n-------------------------------------\nINSTANCE\n-------------------------------------\n";

    config.capture_dir = pt.get<std::string>("captureDir", boost::filesystem::current_path().string());
    size_t pos = config.capture_dir.find_last_of("/");

    if (pos == config.capture_dir.length() - 1) {
      config.capture_dir.erase(config.capture_dir.length() - 1);
    }
    BOOST_LOG_TRIVIAL(info) << "Capture Directory: " << config.capture_dir;
    config.upload_server = pt.get<std::string>("uploadServer", "");
    BOOST_LOG_TRIVIAL(info) << "Upload Server: " << config.upload_server;
    config.bcfy_calls_server = pt.get<std::string>("broadcastifyCallsServer", "");
    BOOST_LOG_TRIVIAL(info) << "Broadcastify Calls Server: " << config.bcfy_calls_server;
    config.status_server = pt.get<std::string>("statusServer", "");
    BOOST_LOG_TRIVIAL(info) << "Status Server: " << config.status_server;
    config.instance_key = pt.get<std::string>("instanceKey", "");
    BOOST_LOG_TRIVIAL(info) << "Instance Key: " << config.instance_key;
    config.instance_id = pt.get<std::string>("instanceId", "");
    BOOST_LOG_TRIVIAL(info) << "Instance Id: " << config.instance_id;
    config.broadcast_signals = pt.get<bool>("broadcastSignals", false);
    BOOST_LOG_TRIVIAL(info) << "Broadcast Signals: " << config.broadcast_signals;
    default_mode = pt.get<std::string>("defaultMode", "digital");
    BOOST_LOG_TRIVIAL(info) << "Default Mode: " << default_mode;
    config.call_timeout = pt.get<int>("callTimeout", 3);
    BOOST_LOG_TRIVIAL(info) << "Call Timeout (seconds): " << config.call_timeout;
    config.log_file = pt.get<bool>("logFile", false);
    BOOST_LOG_TRIVIAL(info) << "Log to File: " << config.log_file;
    config.log_dir = pt.get<std::string>("logDir", "logs");
    BOOST_LOG_TRIVIAL(info) << "Log Directory: " << config.log_dir;
    config.control_message_warn_rate = pt.get<int>("controlWarnRate", 10);
    BOOST_LOG_TRIVIAL(info) << "Control channel warning rate: " << config.control_message_warn_rate;
    config.control_retune_limit = pt.get<int>("controlRetuneLimit", 0);
    BOOST_LOG_TRIVIAL(info) << "Control channel retune limit: " << config.control_retune_limit;

    BOOST_LOG_TRIVIAL(info) << "Frequency format: " << frequencyFormat;

    statusAsString = pt.get<bool>("statusAsString", statusAsString);
    BOOST_LOG_TRIVIAL(info) << "Status as String: " << statusAsString;
    std::string log_level = pt.get<std::string>("logLevel", "info");
    BOOST_LOG_TRIVIAL(info) << "Log Level: " << log_level;
    set_logging_level(log_level);

    BOOST_LOG_TRIVIAL(info) << "\n\n-------------------------------------\nPLUGINS\n-------------------------------------\n";
    //initialize_internal_plugin("stat_socket");
    initialize_plugins(pt, &config, sources, systems);
  } catch (std::exception const &e) {
    BOOST_LOG_TRIVIAL(error) << "Failed parsing Config: " << e.what();
    return false;
  }
  if (config.debug_recorder) {
    BOOST_LOG_TRIVIAL(info) << "\n\n-------------------------------------\nDEBUG RECORDER\n-------------------------------------\n";
    BOOST_LOG_TRIVIAL(info) << "  Address: " << config.debug_recorder_address;

    for (vector<Source *>::iterator it = sources.begin(); it != sources.end(); it++) {
      Source *source = *it;
      BOOST_LOG_TRIVIAL(info) << "  " << source->get_driver() << " - " << source->get_device() << " [ " << FormatFreq(source->get_center()) << " ]  Port: " << source->get_debug_recorder_port();
    }
    BOOST_LOG_TRIVIAL(info) << "\n\n-------------------------------------\n";
  }
  BOOST_LOG_TRIVIAL(info) << "\n\n";
  return true;
}

int get_total_recorders() {
  int total_recorders = 0;

  for (vector<Call *>::iterator it = calls.begin(); it != calls.end(); it++) {
    Call *call = *it;

    if (call->get_state() == RECORDING) {
      total_recorders++;
    }
  }
  return total_recorders;
}

// This maybe needed by websocket_pp
bool replace(std::string &str, const std::string &from, const std::string &to) {
  size_t start_pos = str.find(from);

  if (start_pos == std::string::npos)
    return false;

  str.replace(start_pos, from.length(), to);
  return true;
}

void process_signal(long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder) {
  plugman_signal(unitId, signaling_type, sig_type, call, system, recorder);
}

bool start_recorder(Call *call, TrunkMessage message, System *sys) {
  Talkgroup *talkgroup = sys->find_talkgroup(call->get_talkgroup());
  bool source_found = false;
  bool recorder_found = false;
  Recorder *recorder;
  Recorder *debug_recorder;
  Recorder *sigmf_recorder;

  // BOOST_LOG_TRIVIAL(info) << "\tCall created for: " << call->get_talkgroup()
  // << "\tTDMA: " << call->get_tdma() <<  "\tEncrypted: " <<
  // call->get_encrypted() << "\tFreq: " << call->get_freq();

  if (!talkgroup && (sys->get_record_unknown() == false)) {
    if (sys->get_hideUnknown() == false) {
      BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\tTG: " << call->get_talkgroup_display() << "\tFreq: " << FormatFreq(call->get_freq()) << "\t\u001b[33mNot Recording: TG not in Talkgroup File\u001b[0m ";
    }
    return false;
  }

  if (talkgroup) {
    call->set_talkgroup_tag(talkgroup->alpha_tag);
  } else {
    call->set_talkgroup_tag("-");
  }

  if (call->get_encrypted() == true || (talkgroup && (talkgroup->mode == 'E'))) {
    if (sys->get_hideEncrypted() == false) {
      BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\tTG: " << call->get_talkgroup_display() << "\tFreq: " << FormatFreq(call->get_freq()) << "\t\u001b[31mNot Recording: ENCRYPTED\u001b[0m ";
    }
    return false;
  }

  for (vector<Source *>::iterator it = sources.begin(); it != sources.end(); it++) {
    Source *source = *it;

    if ((source->get_min_hz() <= call->get_freq()) &&
        (source->get_max_hz() >= call->get_freq())) {
      source_found = true;

      if (talkgroup) {
        if (talkgroup->mode == 'A') {
          recorder = source->get_analog_recorder(talkgroup);
        } else {
          recorder = source->get_digital_recorder(talkgroup);
        }
      } else {
        BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\tTG: " << call->get_talkgroup_display() << "\tFreq: " << FormatFreq(call->get_freq()) << "\tTG not in Talkgroup File ";

        // A talkgroup was not found from the talkgroup file.
        // Use an analog recorder if this is a Type II trunk and defaultMode is analog.
        // All other cases use a digital recorder.
        if ((default_mode == "analog") && (sys->get_system_type() == "smartnet")) {
          recorder = source->get_analog_recorder();
        } else {
          recorder = source->get_digital_recorder();
        }
      }
      //int total_recorders = get_total_recorders();

      if (recorder) {
        if (message.meta.length()) {
          BOOST_LOG_TRIVIAL(trace) << message.meta;
        }

        if (recorder->start(call)) {
          call->set_recorder(recorder);
          call->set_state(RECORDING);
          plugman_setup_recorder(recorder);
          recorder_found = true;
        } else {
          recorder_found = false;
          return false;
        }
      } else {
        // not recording call either because the priority was too low or no
        // recorders were available
        return false;
      }

      debug_recorder = source->get_debug_recorder();

      if (debug_recorder) {
        debug_recorder->start(call);
        call->set_debug_recorder(debug_recorder);
        call->set_debug_recording(true);
        plugman_setup_recorder(debug_recorder);
        recorder_found = true;
      } else {
        // BOOST_LOG_TRIVIAL(info) << "\tNot debug recording call";
      }

      sigmf_recorder = source->get_sigmf_recorder();

      if (sigmf_recorder) {
        sigmf_recorder->start(call);
        call->set_sigmf_recorder(sigmf_recorder);
        call->set_sigmf_recording(true);
        plugman_setup_recorder(sigmf_recorder);
        recorder_found = true;
      } else {
        // BOOST_LOG_TRIVIAL(info) << "\tNot debug recording call";
      }

      if (recorder_found) {
        // recording successfully started.
        plugman_call_start(call);
        return true;
      }
    }
  }

  if (!source_found) {
    BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\tTG: " << call->get_talkgroup_display() << "\tFreq: " << FormatFreq(call->get_freq()) << "\t\u001b[36mNot Recording: no source covering Freq\u001b[0m";
    return false;
  }
  return false;
}

// This is to handle the messages that come off the Analog recorder.
void process_message_queues() {
  for (std::vector<System *>::iterator it = systems.begin(); it != systems.end(); ++it) {
    System *sys = (System *)*it;

    for (std::vector<analog_recorder_sptr>::iterator arit = sys->conventional_recorders.begin(); arit != sys->conventional_recorders.end(); ++arit) {
      analog_recorder_sptr ar = (analog_recorder_sptr)*arit;
      ar->process_message_queues();
    }
  }
}

void manage_conventional_call(Call *call) {
  if (call->get_recorder()) {
    // if any recording has happened
    if (call->get_current_length() > 0) {
      BOOST_LOG_TRIVIAL(trace) << "Recorder: " << call->get_current_length() << " Idle: " << call->get_recorder()->is_idle() << " Count: " << call->get_idle_count();

      // means that the squelch is on and it has stopped recording
      if (call->get_recorder()->is_idle()) {
        // increase the number of periods it has not been recording for
        call->increase_idle_count();
      } else if (call->get_idle_count() > 0) {
        // if it starts recording again, then reset the idle count
        call->reset_idle_count();
      }

      // if no additional recording has happened in the past X periods, stop and open new file
      if (call->get_idle_count() > config.call_timeout) {
        Recorder *recorder = call->get_recorder();
        call->stop_call();
        call->conclude_call();
        call->restart_call();
        if (recorder != NULL) {
          plugman_setup_recorder(recorder);
        }
      }
    } else if (!call->get_recorder()->is_active()) {
      // P25 Conventional Recorders need a have the graph unlocked before they can start recording.
      Recorder *recorder = call->get_recorder();
      recorder->start(call);
      call->set_state(RECORDING);
      //plugman_setup_recorder((Recorder *)recorder->get());
    }
  }
}

void print_status() {
  BOOST_LOG_TRIVIAL(info) << "Currently Active Calls: " << calls.size();

  for (vector<Call *>::iterator it = calls.begin(); it != calls.end(); it++) {
    Call *call = *it;
    Recorder *recorder = call->get_recorder();
    BOOST_LOG_TRIVIAL(info) << "TG: " << call->get_talkgroup_display() << " Freq: " << FormatFreq(call->get_freq()) << " Elapsed: " << call->elapsed() << " State: " << FormatState(call->get_state());

    if (recorder) {
      BOOST_LOG_TRIVIAL(info) << "\t[ " << recorder->get_num() << " ] State: " << FormatState(recorder->get_state());
    }
  }

  BOOST_LOG_TRIVIAL(info) << "Recorders: ";

  for (vector<Source *>::iterator it = sources.begin(); it != sources.end(); it++) {
    Source *source = *it;
    source->print_recorders();
  }
}

void manage_calls() {
  bool ended_recording = false;
  for (vector<Call *>::iterator it = calls.begin(); it != calls.end();) {
    Call *call = *it;
    State state = call->get_state();
    // Handle Conventional Calls
    if (call->is_conventional()) {
      manage_conventional_call(call);
      ++it;
      continue;
    }

    // Handle Trunked Calls
    if ((call->since_last_update() > config.call_timeout) && ((state == RECORDING) || (state == MONITORING))) {
      if (state == RECORDING) {
        ended_recording = true;
        call->stop_call();
        ++it;
        continue;
      }
      // we do not need to stop Monitoring Calls, we can just delete them
      if (state == MONITORING) {
        ended_recording = true;
        it = calls.erase(it);
        delete call;
        continue;
      }
    }

    // If a call's state has been set to completed, we can conclude the call and delete it
    if (state == COMPLETED) {

      call->conclude_call();

      // The State of the Recorders has changed, so lets send an update
      ended_recording = true;
      Recorder *recorder = call->get_recorder();
      if (recorder != NULL) {
        plugman_setup_recorder(recorder);
      }
      it = calls.erase(it);
      delete call;
      continue;
    }

    // We are checking to make sure a Call has gotten stuck. If it is in the INACTIVE state
    if (state == INACTIVE) {
      Recorder *recorder = call->get_recorder();
      if (recorder != NULL) {
        if (recorder->since_last_write() > 10) {
          BOOST_LOG_TRIVIAL(info) << "Recorder state: " << recorder->get_state();
          BOOST_LOG_TRIVIAL(info) << "\tTG: " << call->get_talkgroup_display() << "\tFreq: " << FormatFreq(call->get_freq()) << "\t\u001b[36m Removing call with stuck recorder \u001b[0m";
          call->conclude_call();

          // The State of the Recorders has changed, so lets send an update
          ended_recording = true;
          Recorder *recorder = call->get_recorder();
          if (recorder != NULL) {
            plugman_setup_recorder(recorder);
          }
          it = calls.erase(it);
          delete call;
          continue;
        }

        if ((recorder->get_state() == IDLE) || (recorder->get_state() == COMPLETED)) {
          BOOST_LOG_TRIVIAL(info) << "Recorder state: " << recorder->get_state();
          BOOST_LOG_TRIVIAL(info) << "\tTG: " << call->get_talkgroup_display() << "\tFreq: " << FormatFreq(call->get_freq()) << "\t\u001b[36m Recorder completed or gone idle \u001b[0m";

          call->conclude_call();

          // The State of the Recorders has changed, so lets send an update
          ended_recording = true;
          Recorder *recorder = call->get_recorder();
          if (recorder != NULL) {
            plugman_setup_recorder(recorder);
          }
          it = calls.erase(it);
          delete call;
          continue;
        }
      } else {
        BOOST_LOG_TRIVIAL(error) << "\tTG: " << call->get_talkgroup_display() << "\tFreq: " << FormatFreq(call->get_freq()) << "\t\u001b[36m Call set to Inactive, but has no recorder\u001b[0m";
      }
    }

    ++it;
    // if rx is active
  } // foreach loggers

  if (ended_recording) {
    plugman_calls_active(calls);
  }
}

void current_system_status(TrunkMessage message, System *sys) {
  if (sys->update_status(message)) {
    plugman_setup_system(sys);
  }
}

void unit_registration(System *sys, long source_id) {
  plugman_unit_registration(sys, source_id);
}

void unit_deregistration(System *sys, long source_id) {
  plugman_unit_deregistration(sys, source_id);
}

void unit_acknowledge_response(System *sys, long source_id) {
  plugman_unit_acknowledge_response(sys, source_id);
}

void unit_group_affiliation(System *sys, long source_id, long talkgroup_num) {
  plugman_unit_group_affiliation(sys, source_id, talkgroup_num);
}

void handle_call(TrunkMessage message, System *sys) {
  bool call_found = false;
  bool call_retune = false;
  bool recording_started = false;

  /* Notes: it is possible for 2 Calls to exist for the same talkgroup on different freq. This happens when a Talkgroup starts on a freq
  that current recorder can't retune to. In this case, the current orig Talkgroup reocrder will keep going on the old freq, while a new
  recorder is start on a source that can cover that freq. This makes sure any of the remaining transmission that it is in the buffer
  of the original recorder gets flushed. */

  for (vector<Call *>::iterator it = calls.begin(); it != calls.end(); ++it) {
    Call *call = *it;

    if ((call->get_state() != RECORDING) && (call->get_state() != MONITORING)) {
      continue;
    }

    //BOOST_LOG_TRIVIAL(info) << "TG: " << call->get_talkgroup() << " | " << message.talkgroup << " sys num: " << call->get_sys_num() << " | " << message.sys_num << " freq: " << call->get_freq() << " | " << message.freq << " TDMA Slot" << call->get_tdma_slot() << " | " << message.tdma_slot << " TDMA: " << call->get_phase2_tdma() << " | " << message.phase2_tdma;
    if ((call->get_talkgroup() == message.talkgroup) && (call->get_sys_num() == message.sys_num) && (call->get_freq() == message.freq) && (call->get_tdma_slot() == message.tdma_slot) && (call->get_phase2_tdma() == message.phase2_tdma)) {
      call_found = true;
      call->update(message);
      break;
    }
  }

  if (!call_found) {
    Call *call = new Call(message, sys, config);
    recording_started = start_recorder(call, message, sys);
    calls.push_back(call);
  }

  if (recording_started) {
    plugman_calls_active(calls);
  }
}

void handle_message(std::vector<TrunkMessage> messages, System *sys) {
  for (std::vector<TrunkMessage>::iterator it = messages.begin(); it != messages.end(); it++) {
    TrunkMessage message = *it;

    switch (message.message_type) {
    case GRANT:
    case UPDATE:
      handle_call(message, sys);
      break;

    case CONTROL_CHANNEL:
      sys->add_control_channel(message.freq);
      break;

    case REGISTRATION:
      unit_registration(sys, message.source);
      break;

    case DEREGISTRATION:
      unit_deregistration( sys, message.source);
      break;

    case AFFILIATION:
      unit_group_affiliation( sys, message.source, message.talkgroup);
      break;

    case SYSID:
      break;

    case STATUS:
      current_system_status(message, sys);
      break;

    case ACKRESP:
      unit_acknowledge_response( sys, message.source);
      break;

    case UNKNOWN:
      break;
    }
  }
}

System *find_system(int sys_num) {
  System *sys_match = NULL;

  for (std::vector<System *>::iterator it = systems.begin(); it != systems.end(); ++it) {
    System *sys = (System *)*it;

    if (sys->get_sys_num() == sys_num) {
      sys_match = sys;
      break;
    }
  }

  if (sys_match == NULL) {
    BOOST_LOG_TRIVIAL(info) << "Sys is null";
  }
  return sys_match;
}

void retune_system(System *system) {
  bool source_found = false;
  Source *current_source = system->get_source();
  double control_channel_freq = system->get_next_control_channel();

  BOOST_LOG_TRIVIAL(error) << "[" << system->get_short_name() << "] Retuning to Control Channel: " << FormatFreq(control_channel_freq);

  if ((current_source->get_min_hz() <= control_channel_freq) &&
      (current_source->get_max_hz() >= control_channel_freq)) {
    source_found = true;
    BOOST_LOG_TRIVIAL(info) << "\t - System Source " << current_source->get_num() << " - Min Freq: " << FormatFreq(current_source->get_min_hz()) << " Max Freq: " << FormatFreq(current_source->get_max_hz());
    // The source can cover the System's control channel, break out of the
    // For Loop
    if (system->get_system_type() == "smartnet") {
      system->smartnet_trunking->tune_freq(control_channel_freq);
      system->smartnet_trunking->reset();
    } else if (system->get_system_type() == "p25") {
      system->p25_trunking->tune_freq(control_channel_freq);
    } else {
      BOOST_LOG_TRIVIAL(error) << "\t - Unknown system type for Retune";
    }
  } else {
    for (vector<Source *>::iterator src_it = sources.begin(); src_it != sources.end(); src_it++) {
      Source *source = *src_it;

      if ((source->get_min_hz() <= control_channel_freq) &&
          (source->get_max_hz() >= control_channel_freq)) {
        source_found = true;
        BOOST_LOG_TRIVIAL(info) << "\t - System Source " << source->get_num() << " - Min Freq: " << FormatFreq(source->get_min_hz()) << " Max Freq: " << FormatFreq(source->get_max_hz());

        if (system->get_system_type() == "smartnet") {
          system->set_source(source);
          // We must lock the flow graph in order to disconnect and reconnect blocks
          tb->lock();
          tb->disconnect(current_source->get_src_block(), 0, system->smartnet_trunking, 0);
          tb->connect(source->get_src_block(), 0, system->smartnet_trunking, 0);
          tb->unlock();
          system->smartnet_trunking->set_center(source->get_center());
          system->smartnet_trunking->set_rate(source->get_rate());
          system->smartnet_trunking->tune_freq(control_channel_freq);
          system->smartnet_trunking->reset();
        } else if (system->get_system_type() == "p25") {
          system->set_source(source);
          // We must lock the flow graph in order to disconnect and reconnect blocks
          tb->lock();
          tb->disconnect(current_source->get_src_block(), 0, system->p25_trunking, 0);
          tb->connect(source->get_src_block(), 0, system->p25_trunking, 0);
          tb->unlock();
          system->p25_trunking->set_center(source->get_center());
          system->p25_trunking->set_rate(source->get_rate());
          system->p25_trunking->tune_freq(control_channel_freq);
        } else {
          BOOST_LOG_TRIVIAL(error) << "\t - Unkown system type for Retune";
        }

        // break out of the For Loop
        break;
      }
    }
  }
  if (!source_found) {
    BOOST_LOG_TRIVIAL(error) << "\t - Unable to retune System control channel, freq not covered by any source.";
  }
}

void check_message_count(float timeDiff) {
  plugman_setup_config(sources, systems);
  plugman_system_rates(systems, timeDiff);

  for (std::vector<System *>::iterator it = systems.begin(); it != systems.end(); ++it) {
    System *sys = (System *)*it;

    if ((sys->system_type != "conventional") && (sys->system_type != "conventionalP25")) {
      float msgs_decoded_per_second = sys->message_count / timeDiff;

      if (msgs_decoded_per_second < 2) {

        if (sys->control_channel_count() > 1) {
          retune_system(sys);
        } else {
          BOOST_LOG_TRIVIAL(error) << "[" << sys->get_short_name() << "]\tThere is only one control channel defined";
        }

        // if it loses track of the control channel, quit after a while
        if (config.control_retune_limit > 0) {
          sys->retune_attempts++;
          if (sys->retune_attempts > config.control_retune_limit) {
            exit_flag = 1;
          }
        }
      } else {
        sys->retune_attempts = 0;
      }

      if (msgs_decoded_per_second < config.control_message_warn_rate || config.control_message_warn_rate == -1) {
        BOOST_LOG_TRIVIAL(error) << "[" << sys->get_short_name() << "]\t Control Channel Message Decode Rate: " << msgs_decoded_per_second << "/sec, count:  " << sys->message_count;
      }
    }
    sys->message_count = 0;
  }
}

void monitor_messages() {
  gr::message::sptr msg;
  int sys_num;
  System *sys;

  time_t lastStatusTime = time(NULL);
  time_t lastMsgCountTime = time(NULL);
  time_t management_timestamp = time(NULL);
  time_t current_time = time(NULL);
  std::vector<TrunkMessage> trunk_messages;

  while (1) {

    if (exit_flag) { // my action when signal set it 1
      printf("\n Signal caught!\n");
      return;
    }

    process_message_queues();

    plugman_poll_one();


    msg = msg_queue->delete_head_nowait();

    if (msg != 0) {
      sys_num = msg->arg1();
      sys = find_system(sys_num);

      if (sys) {
        sys->message_count++;

        if (sys->get_system_type() == "smartnet") {
          trunk_messages = smartnet_parser->parse_message(msg->to_string(), sys);
          handle_message(trunk_messages, sys);
        }

        if (sys->get_system_type() == "p25") {
          trunk_messages = p25_parser->parse_message(msg);
          handle_message(trunk_messages, sys);
        }
      }

      msg.reset();
    } else {
      current_time = time(NULL);

      if ((current_time - management_timestamp) >= 1.0) {
        manage_calls();
        Call_Concluder::manage_call_data_workers();
        management_timestamp = current_time;
      }

      boost::this_thread::sleep(boost::posix_time::milliseconds(10));
      //usleep(1000 * 10);
    }

    current_time = time(NULL);

    float timeDiff = current_time - lastMsgCountTime;

    if (timeDiff >= 3.0) {
      check_message_count(timeDiff);
      lastMsgCountTime = current_time;
    }

    float statusTimeDiff = current_time - lastStatusTime;

    if (statusTimeDiff > 200) {
      lastStatusTime = current_time;
      print_status();
    }
  }
}

bool setup_systems() {
  bool system_added = false;
  Source *source = NULL;

  for (vector<System *>::iterator sys_it = systems.begin(); sys_it != systems.end(); sys_it++) {
    System *system = *sys_it;
    //bool    source_found = false;

    if ((system->get_system_type() == "conventional") || (system->get_system_type() == "conventionalP25")) {
      std::vector<double> channels = system->get_channels();
      int tg_iterate_index = 0;

      for (vector<double>::iterator chan_it = channels.begin(); chan_it != channels.end(); chan_it++) {
        double channel = *chan_it;
        ++tg_iterate_index;
        bool channel_added = false;

        for (vector<Source *>::iterator src_it = sources.begin(); src_it != sources.end(); src_it++) {
          source = *src_it;

          if ((source->get_min_hz() <= channel) && (source->get_max_hz() >= channel)) {
            channel_added = true;
            if (system->get_squelch_db() == -160) {
              BOOST_LOG_TRIVIAL(error) << "[" << system->get_short_name() << "]\tSquelch needs to be specified for the Source for Conventional Systems";
              system_added = false;
            } else {
              system_added = true;
            }

            // This source can be used for this channel (and a squelch is set)
            BOOST_LOG_TRIVIAL(info) << "[" << system->get_short_name() << "]\tMonitoring Conventional Channel: " << FormatFreq(channel) << " Talkgroup: " << tg_iterate_index;
            Call_conventional *call = new Call_conventional(tg_iterate_index, channel, system, config);
            Talkgroup *talkgroup = system->find_talkgroup(call->get_talkgroup());

            if (talkgroup) {
              call->set_talkgroup_tag(talkgroup->alpha_tag);
            }

            if (system->get_system_type() == "conventional") {
              analog_recorder_sptr rec;
              rec = source->create_conventional_recorder(tb);
              rec->start(call);
              call->set_recorder((Recorder *)rec.get());
              call->set_state(RECORDING);
              system->add_conventional_recorder(rec);
              calls.push_back(call);
              plugman_setup_recorder((Recorder *)rec.get());
            } else { // has to be "conventional P25"
              // Because of dynamic mod assignment we can not start the recorder until the graph has been unlocked.
              // This has something to do with the way the Selector block works.
              // the manage_calls() function handles adding and starting the P25 Recorder
              p25_recorder_sptr rec;
              rec = source->create_digital_conventional_recorder(tb);
              call->set_recorder((Recorder *)rec.get());
              calls.push_back(call);
            }

            // break out of the for loop
            break;
          }
        }
        if (!channel_added) {
          BOOST_LOG_TRIVIAL(error) << "[" << system->get_short_name() << "]\t Unable to find a source for this conventional channel! Channel not added: " << FormatFreq(channel) << " Talkgroup: " << tg_iterate_index;
        }
      }
    } else {
      // If it's not a conventional system, then it's a trunking system
      double control_channel_freq = system->get_current_control_channel();
      BOOST_LOG_TRIVIAL(info) << "[" << system->get_short_name() << "]\tStarted with Control Channel: " << FormatFreq(control_channel_freq);

      for (vector<Source *>::iterator src_it = sources.begin(); src_it != sources.end(); src_it++) {
        source = *src_it;

        if ((source->get_min_hz() <= control_channel_freq) &&
            (source->get_max_hz() >= control_channel_freq)) {
          // The source can cover the System's control channel
          system_added = true;
          system->set_source(source);

          if (system->get_system_type() == "smartnet") {
            // what you really need to do is go through all of the sources to find
            // the one with the right frequencies
            system->smartnet_trunking = make_smartnet_trunking(control_channel_freq,
                                                               source->get_center(),
                                                               source->get_rate(),
                                                               msg_queue,
                                                               system->get_sys_num());
            tb->connect(source->get_src_block(), 0, system->smartnet_trunking, 0);
          }

          if (system->get_system_type() == "p25") {
            // what you really need to do is go through all of the sources to find
            // the one with the right frequencies
            system->p25_trunking = make_p25_trunking(control_channel_freq,
                                                     source->get_center(),
                                                     source->get_rate(),
                                                     msg_queue,
                                                     system->get_qpsk_mod(),
                                                     system->get_sys_num());
            tb->connect(source->get_src_block(), 0, system->p25_trunking, 0);
          }

          // break out of the For Loop
          break;
        }
      }
    }
  }
  return system_added;
}

template <class F>
void add_logs(const F &fmt) {
  boost::shared_ptr<sinks::synchronous_sink<sinks::basic_text_ostream_backend<char>>> sink =
      boost::log::add_console_log(std::clog, boost::log::keywords::format = fmt);

  std::locale loc = std::locale("C");

  sink->imbue(loc);
}

int main(int argc, char **argv) {
  //BOOST_STATIC_ASSERT(true) __attribute__((unused));

  logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::info);

  boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");

  boost::log::add_common_attributes();
  boost::log::core::get()->add_global_attribute("Scope", boost::log::attributes::named_scope());
  boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);

  add_logs(boost::log::expressions::format("[%1%] (%2%)   %3%") % boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f") % boost::log::expressions::attr<boost::log::trivial::severity_level>("Severity") % boost::log::expressions::smessage);

  //boost::log::sinks->imbue(std::locale("C"));
  //std::locale::global(std::locale("C"));

  boost::program_options::options_description desc("Options");
  desc.add_options()("help,h", "Help screen")("config", boost::program_options::value<string>()->default_value("./config.json"), "Config File");
  boost::program_options::variables_map vm;
  boost::program_options::store(parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);

  if (vm.count("help")) {
    std::cout << "Usage: options_description [options]\n";
    std::cout << desc;
    exit(0);
  }
  string config_file = vm["config"].as<string>();

  if (vm.count("config")) {
    BOOST_LOG_TRIVIAL(info) << "Using Config file: " << config_file << "\n";
  }

  tb = gr::make_top_block("Trunking");
  tb->start();
  tb->lock();
  msg_queue = gr::msg_queue::make(100);
  smartnet_parser = new SmartnetParser(); // this has to eventually be generic;
  p25_parser = new P25Parser();

  std::string uri = "ws://localhost:3005";

  if (!load_config(config_file)) {
    tb->unlock();
    tb->stop();
    tb->wait();
    exit(1);
  }

  start_plugins(sources, systems);

  if (config.log_file) {
    logging::add_file_log(
        keywords::file_name = config.log_dir + "/%m-%d-%Y_%H%M_%2N.log",
        keywords::format = "[%TimeStamp%] (%Severity%)   %Message%",
        keywords::rotation_size = 10 * 1024 * 1024,
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
        keywords::auto_flush = true);
  }

  if (setup_systems()) {
    signal(SIGINT, exit_interupt);
    tb->unlock();

    monitor_messages();

    // ------------------------------------------------------------------
    // -- stop flow graph execution
    // ------------------------------------------------------------------
    BOOST_LOG_TRIVIAL(info) << "stopping flow graph" << std::endl;
    tb->stop();
    tb->wait();

    BOOST_LOG_TRIVIAL(info) << "stopping plugins" << std::endl;
    stop_plugins();
  } else {
    BOOST_LOG_TRIVIAL(error) << "Unable to setup a System to record, exiting..." << std::endl;
  }

  return 1;
}
