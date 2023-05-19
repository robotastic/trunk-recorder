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
#include <utility>
#include <cmath>

#include "./global_structs.h"
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
#include "systems/system_impl.h"

#include <osmosdr/source.h>

#include "formatter.h"
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/gr_complex.h>
#include <gnuradio/message.h>
#include <gnuradio/top_block.h>
#include <gnuradio/uhd/usrp_source.h>

#include "plugin_manager/plugin_manager.h"

#include "cmake.h"
#include "git.h"

using namespace std;
namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
std::vector<Source *> sources;
std::vector<System *> systems;
std::map<long, long> unit_affiliations;

std::vector<Call *> calls;

gr::top_block_sptr tb;


volatile sig_atomic_t exit_flag = 0;
int exit_code = EXIT_SUCCESS;
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
    config.log_file = pt.get<bool>("logFile", false);
    BOOST_LOG_TRIVIAL(info) << "Log to File: " << config.log_file;
    config.log_dir = pt.get<std::string>("logDir", "logs");
    BOOST_LOG_TRIVIAL(info) << "Log Directory: " << config.log_dir;
    if (config.log_file) {
      logging::add_file_log(
          keywords::file_name = config.log_dir + "/%m-%d-%Y_%H%M_%2N.log",
          keywords::format = "[%TimeStamp%] (%Severity%)   %Message%",
          keywords::rotation_size = 100 * 1024 * 1024,
          keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
          keywords::auto_flush = true);
    }
    BOOST_LOG_TRIVIAL(info) << "\n-------------------------------------\n     Trunk Recorder\n-------------------------------------\n";

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
    config.control_message_warn_rate = pt.get<int>("controlWarnRate", 10);
    BOOST_LOG_TRIVIAL(info) << "Control channel warning rate: " << config.control_message_warn_rate;
    config.control_retune_limit = pt.get<int>("controlRetuneLimit", 0);
    BOOST_LOG_TRIVIAL(info) << "Control channel retune limit: " << config.control_retune_limit;
    config.enable_audio_streaming = pt.get<bool>("audioStreaming", false);
    BOOST_LOG_TRIVIAL(info) << "Enable Audio Streaming: " << config.enable_audio_streaming;
    config.record_uu_v_calls = pt.get<bool>("recordUUVCalls", true);
    BOOST_LOG_TRIVIAL(info) << "Record Unit to Unit Voice Calls: " << config.record_uu_v_calls;
    std::string frequency_format_string = pt.get<std::string>("frequencyFormat", "mhz");

    if (boost::iequals(frequency_format_string, "mhz")) {
      frequency_format = 1;
    } else if (boost::iequals(frequency_format_string, "hz")) {
      frequency_format = 2;
    } else {
      frequency_format = 0;
    }
    config.frequency_format = frequency_format;
    BOOST_LOG_TRIVIAL(info) << "Frequency format: " << get_frequency_format();

    statusAsString = pt.get<bool>("statusAsString", statusAsString);
    BOOST_LOG_TRIVIAL(info) << "Status as String: " << statusAsString;
    std::string log_level = pt.get<std::string>("logLevel", "info");
    BOOST_LOG_TRIVIAL(info) << "Log Level: " << log_level;
    set_logging_level(log_level);

    BOOST_LOG_TRIVIAL(info) << "\n-------------------------------------\nSYSTEMS\n-------------------------------------\n";

    config.debug_recorder = pt.get<bool>("debugRecorder", 0);
    config.debug_recorder_address = pt.get<std::string>("debugRecorderAddress", "127.0.0.1");
    config.debug_recorder_port = pt.get<int>("debugRecorderPort", 1234);

    BOOST_FOREACH (boost::property_tree::ptree::value_type &node,
                   pt.get_child("systems")) {
      bool system_enabled = node.second.get<bool>("enabled", true);
      if (system_enabled) {
        // each system should have a unique index value;
        System *system = System::make(sys_count++);

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

        // If it is a conventional System
        if ((system->get_system_type() == "conventional") || (system->get_system_type() == "conventionalP25") || (system->get_system_type() == "conventionalDMR")) {

          boost::optional<std::string> channel_file_exist = node.second.get_optional<std::string>("channelFile");
          boost::optional<boost::property_tree::ptree &> channels_exist = node.second.get_child_optional("channels");

          if (channel_file_exist && channels_exist) {
            BOOST_LOG_TRIVIAL(error) << "Both \"channels\" and \"channelFile\" cannot be defined for a system!";
            return false;
          }

          if (channels_exist) {
            BOOST_LOG_TRIVIAL(info) << "Conventional Channels: ";
            BOOST_FOREACH (boost::property_tree::ptree::value_type &sub_node, node.second.get_child("channels")) {
              double channel = sub_node.second.get<double>("", 0);

              BOOST_LOG_TRIVIAL(info) << "  " << format_freq(channel);
              system->add_channel(channel);
            }
          } else if (channel_file_exist) {
            std::string channel_file = node.second.get<std::string>("channelFile");
            BOOST_LOG_TRIVIAL(info) << "Channel File: " << channel_file;
            system->set_channel_file(channel_file);
          } else {
            BOOST_LOG_TRIVIAL(error) << "Either \"channels\" or \"channelFile\" need to be defined for a conventional system!";
            return false;
          }
          // If it is a Trunked System
        } else if ((system->get_system_type() == "smartnet") || (system->get_system_type() == "p25")) {
          BOOST_LOG_TRIVIAL(info) << "Control Channels: ";
          BOOST_FOREACH (boost::property_tree::ptree::value_type &sub_node, node.second.get_child("control_channels")) {
            double control_channel = sub_node.second.get<double>("", 0);
            BOOST_LOG_TRIVIAL(info) << "  " << format_freq(control_channel);
            system->add_control_channel(control_channel);
          }
          system->set_talkgroups_file(node.second.get<std::string>("talkgroupsFile", ""));
          BOOST_LOG_TRIVIAL(info) << "Talkgroups File: " << system->get_talkgroups_file();
        } else {
          BOOST_LOG_TRIVIAL(error) << "System Type in config.json not recognized";
          return false;
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
        BOOST_LOG_TRIVIAL(info) << "Upload Script: " << system->get_upload_script();
        system->set_compress_wav(node.second.get<bool>("compressWav", true));
        BOOST_LOG_TRIVIAL(info) << "Compress .wav Files: " << system->get_compress_wav();
        system->set_call_log(node.second.get<bool>("callLog", true));
        BOOST_LOG_TRIVIAL(info) << "Call Log: " << system->get_call_log();
        system->set_audio_archive(node.second.get<bool>("audioArchive", true));
        BOOST_LOG_TRIVIAL(info) << "Audio Archive: " << system->get_audio_archive();
        system->set_transmission_archive(node.second.get<bool>("transmissionArchive", false));
        BOOST_LOG_TRIVIAL(info) << "Transmission Archive: " << system->get_transmission_archive();
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
          system->set_talkgroup_display_format(talkGroupDisplayFormat_id_tag);
        } else if (boost::iequals(talkgroup_display_format_string, "tag_id")) {
          system->set_talkgroup_display_format(talkGroupDisplayFormat_tag_id);
        } else {
          system->set_talkgroup_display_format(talkGroupDisplayFormat_id);
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
        system->set_max_duration(node.second.get<double>("maxDuration", 0));
        BOOST_LOG_TRIVIAL(info) << "Maximum Call Duration (in seconds): " << system->get_max_duration();
        system->set_min_tx_duration(node.second.get<double>("minTransmissionDuration", 0));
        BOOST_LOG_TRIVIAL(info) << "Minimum Transmission Duration (in seconds): " << system->get_min_tx_duration();
        system->set_multiSite(node.second.get<bool>("multiSite", false));
        BOOST_LOG_TRIVIAL(info) << "Multiple Site System: " << system->get_multiSite();
        system->set_multiSiteSystemName(node.second.get<std::string>("multiSiteSystemName", ""));
        BOOST_LOG_TRIVIAL(info) << "Multiple Site System Name: " << system->get_multiSiteSystemName();
        system->set_multiSiteSystemNumber(node.second.get<unsigned long>("multiSiteSystemNumber", 0));
        BOOST_LOG_TRIVIAL(info) << "Multiple Site System Number: " << system->get_multiSiteSystemNumber();

        if (!system->get_compress_wav()) {
          if ((system->get_api_key().length() > 0) || (system->get_bcfy_api_key().length() > 0)) {
            BOOST_LOG_TRIVIAL(error) << "Compress WAV must be set to true if you are using OpenMHz or Broadcastify";
            return false;
          }
        }

        systems.push_back(system);
        BOOST_LOG_TRIVIAL(info);
      }
    }

    BOOST_LOG_TRIVIAL(info) << "\n\n-------------------------------------\nSOURCES\n-------------------------------------\n";
    BOOST_FOREACH (boost::property_tree::ptree::value_type &node,
                   pt.get_child("sources")) {
      bool source_enabled = node.second.get<bool>("enabled", true);
      if (source_enabled) {
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
        int amp_gain = node.second.get<double>("ampGain", 0);
        int vga_gain = node.second.get<double>("vgaGain", 0);
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
        BOOST_LOG_TRIVIAL(info) << "Center: " << format_freq(node.second.get<double>("center", 0));
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
        BOOST_LOG_TRIVIAL(info) << "AMP Gain: " << node.second.get<double>("ampGain", 0);
        BOOST_LOG_TRIVIAL(info) << "VGA Gain: " << node.second.get<double>("vgaGain", 0);
        BOOST_LOG_TRIVIAL(info) << "VGA1 Gain: " << node.second.get<double>("vga1Gain", 0);
        BOOST_LOG_TRIVIAL(info) << "VGA2 Gain: " << node.second.get<double>("vga2Gain", 0);
        BOOST_LOG_TRIVIAL(info) << "Idle Silence: " << node.second.get<bool>("silenceFrame", 0);
        BOOST_LOG_TRIVIAL(info) << "Digital Recorders: " << node.second.get<int>("digitalRecorders", 0);
        BOOST_LOG_TRIVIAL(info) << "SigMF Recorders: " << node.second.get<int>("sigmfRecorders", 0);
        BOOST_LOG_TRIVIAL(info) << "Analog Recorders: " << node.second.get<int>("analogRecorders", 0);

        if ((ppm != 0) && (error != 0)) {
          BOOST_LOG_TRIVIAL(info) << "Both PPM and Error should not be set at the same time. Setting Error to 0.";
          error = 0;
        }

        Source *source = new Source(center, rate, error, driver, device, &config);
        BOOST_LOG_TRIVIAL(info) << "Max Frequency: " << format_freq(source->get_max_hz());
        BOOST_LOG_TRIVIAL(info) << "Min Frequency: " << format_freq(source->get_min_hz());
        if (node.second.count("gainSettings") != 0) {
          BOOST_FOREACH (boost::property_tree::ptree::value_type &sub_node, node.second.get_child("gainSettings")) {
            source->set_gain_by_name(sub_node.first, sub_node.second.get<double>("", 0));
            gain_set = true;
          }
        }

        if (if_gain != 0) {
          gain_set = true;
          source->set_gain_by_name("IF", if_gain);
        }

        if (bb_gain != 0) {
          gain_set = true;
          source->set_gain_by_name("BB", bb_gain);
        }

        if (mix_gain != 0) {
          gain_set = true;
          source->set_gain_by_name("MIX", mix_gain);
        }

        if (lna_gain != 0) {
          gain_set = true;
          source->set_gain_by_name("LNA", lna_gain);
        }

        if (tia_gain != 0) {
          gain_set = true;
          source->set_gain_by_name("TIA", tia_gain);
        }

        if (pga_gain != 0) {
          gain_set = true;
          source->set_gain_by_name("PGA", pga_gain);
        }

        if (amp_gain != 0) {
          gain_set = true;
          source->set_gain_by_name("AMP", amp_gain);
        }

        if (vga_gain != 0) {
          gain_set = true;
          source->set_gain_by_name("VGA", vga_gain);
        }

        if (vga1_gain != 0) {
          gain_set = true;
          source->set_gain_by_name("VGA1", vga1_gain);
        }

        if (vga2_gain != 0) {
          gain_set = true;
          source->set_gain_by_name("VGA2", vga2_gain);
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
    }

    BOOST_LOG_TRIVIAL(info) << "\n\n-------------------------------------\nPLUGINS\n-------------------------------------\n";
    add_internal_plugin("openmhz_uploader", "libopenmhz_uploader.so", pt);
    add_internal_plugin("broadcastify_uploader", "libbroadcastify_uploader.so", pt);
    add_internal_plugin("unit_script", "libunit_script.so", pt);
    add_internal_plugin("stat_socket", "libstat_socket.so", pt);
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
      BOOST_LOG_TRIVIAL(info) << "  " << source->get_driver() << " - " << source->get_device() << " [ " << format_freq(source->get_center()) << " ]  Port: " << source->get_debug_recorder_port();
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

  if (!talkgroup && (sys->get_record_unknown() == false)) {
    if (sys->get_hideUnknown() == false) {
      // BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[33mNot Recording: TG not in Talkgroup File\u001b[0m ";
    }
    return false;
  }

  if (talkgroup) {
    call->set_talkgroup_tag(talkgroup->alpha_tag);
  } else {
    call->set_talkgroup_tag("-");
  }

  if (call->get_encrypted() == true || (talkgroup && (talkgroup->mode.compare("E") == 0 || talkgroup->mode.compare("TE") == 0 || talkgroup->mode.compare("DE") == 0))) {
    call->set_state(MONITORING);
    call->set_monitoring_state(ENCRYPTED);
    if (sys->get_hideEncrypted() == false) {
      long unit_id = call->get_current_source_id();
      std::string tag = sys->find_unit_tag(unit_id);
      if (tag != "") {
        tag = " (\033[0;34m" + tag + "\033[0m)";
      }
      BOOST_LOG_TRIVIAL(debug) << "[" << sys->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[31mNot Recording: ENCRYPTED\u001b[0m - src: " << unit_id << tag;
    }
    return false;
  }

  for (vector<Source *>::iterator it = sources.begin(); it != sources.end(); it++) {
    Source *source = *it;

    if ((source->get_min_hz() <= call->get_freq()) &&
        (source->get_max_hz() >= call->get_freq())) {
      source_found = true;

      if (talkgroup) {
        int priority = talkgroup->get_priority();
        BOOST_FOREACH (auto &TGID, sys->get_talkgroup_patch(call->get_talkgroup())) {
          if (sys->find_talkgroup(TGID) != NULL) {
            if (sys->find_talkgroup(TGID)->get_priority() < priority) {
              priority = sys->find_talkgroup(TGID)->get_priority();
              BOOST_LOG_TRIVIAL(info) << "Temporarily increased priority of talkgroup " << call->get_talkgroup() << " to " << sys->find_talkgroup(TGID)->get_priority() << " due to active patch with talkgroup " << TGID;
            }
          }
        }
        if (talkgroup->mode.compare("A") == 0) {
          recorder = source->get_analog_recorder(talkgroup, priority, call);
          call->set_is_analog(true);
        } else {
          recorder = source->get_digital_recorder(talkgroup, priority, call);
        }
      } else {
        BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\tTG not in Talkgroup File ";

        // A talkgroup was not found from the talkgroup file.
        // Use an analog recorder if this is a Type II trunk and defaultMode is analog.
        // All other cases use a digital recorder.
        if ((default_mode == "analog") && (sys->get_system_type() == "smartnet")) {
          recorder = source->get_analog_recorder(call);
          call->set_is_analog(true);
        } else {
          recorder = source->get_digital_recorder(call);
        }
      }
      // int total_recorders = get_total_recorders();

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
          call->set_state(MONITORING);
          // call->set_monitoring_state(NO_SOURCE);
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
        // BOOST_LOG_TRIVIAL(info) << "\tNot SIGMF recording call";
      }

      if (recorder_found) {
        // recording successfully started.
        return true;
      }
    }
  }

  if (!source_found) {
    call->set_state(MONITORING);
    call->set_monitoring_state(NO_SOURCE);
    BOOST_LOG_TRIVIAL(debug) << "[" << sys->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[36mNot Recording: no source covering Freq\u001b[0m";
    return false;
  }
  return false;
}

// This is to handle the messages that come off the Analog recorder.
void process_message_queues() {
  for (std::vector<System *>::iterator it = systems.begin(); it != systems.end(); ++it) {
    System_impl *sys = (System_impl *)*it;

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
      BOOST_LOG_TRIVIAL(trace) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m Call Length: " << call->get_current_length() << "s\t Idle: " << call->get_recorder()->is_idle() << "\t Idle Count: " << call->get_idle_count();

      // means that the squelch is on and it has stopped recording
      if (call->get_recorder()->is_squelched()) {
        // increase the number of periods it has not been recording for
        call->increase_idle_count();
      } else if (call->get_idle_count() > 0) {
        // if it starts recording again, then reset the idle count
        call->reset_idle_count();
      }

      // if no additional recording has happened in the past X periods, stop and open new file
      if (call->get_idle_count() > config.call_timeout) {
        Recorder *recorder = call->get_recorder();
        call->set_state(COMPLETED);
        call->conclude_call();
        call->restart_call();
        if (recorder != NULL) {
          plugman_setup_recorder(recorder);
          plugman_call_start(call);
        }
      } else if ((call->get_current_length() > call->get_system()->get_max_duration()) && (call->get_system()->get_max_duration() > 0)) {
        Recorder *recorder = call->get_recorder();
        call->set_state(COMPLETED);
        call->conclude_call();
        call->restart_call();
        if (recorder != NULL) {
          plugman_setup_recorder(recorder);
          plugman_call_start(call);
        }
      }
    } else if (!call->get_recorder()->is_active()) {
      // P25 Conventional and DMR Recorders need a have the graph unlocked before they can start recording.
      Recorder *recorder = call->get_recorder();
      recorder->start(call);
      call->set_state(RECORDING);
      plugman_call_start(call);
      BOOST_LOG_TRIVIAL(trace) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m Starting P25 Convetional Recorder ";

      // plugman_setup_recorder((Recorder *)recorder->get());
    }
  }
}

void print_status() {
  BOOST_LOG_TRIVIAL(info) << "Currently Active Calls: " << calls.size();

  for (vector<Call *>::iterator it = calls.begin(); it != calls.end(); it++) {
    Call *call = *it;
    Recorder *recorder = call->get_recorder();

    if (call->get_state() == MONITORING) {
      BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m TG: " << call->get_talkgroup_display() << " Freq: " << format_freq(call->get_freq()) << " Elapsed: " << std::setw(4) << call->elapsed() << " State: " << format_state(call->get_state(), call->get_monitoring_state());
    } else {
      BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m TG: " << call->get_talkgroup_display() << " Freq: " << format_freq(call->get_freq()) << " Elapsed: " << std::setw(4) << call->elapsed() << " State: " << format_state(call->get_state());
    }

    if (recorder) {
      BOOST_LOG_TRIVIAL(info) << "\t[ " << recorder->get_num() << " ] State: " << format_state(recorder->get_state());
    }
  }

  
  BOOST_LOG_TRIVIAL(info) << "Recorders: ";

  for (vector<Source *>::iterator it = sources.begin(); it != sources.end(); it++) {
    Source *source = *it;
    source->print_recorders();
  }

    BOOST_LOG_TRIVIAL(info) << "Control Channel Decode Rates: ";
      for (std::vector<System *>::iterator it = systems.begin(); it != systems.end(); ++it) {
    System_impl *sys = (System_impl *)*it;

      if ((sys->get_system_type() != "conventional") && (sys->get_system_type() != "conventionalP25") && (sys->get_system_type() != "conventionalDMR")) {
        BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "] " << sys->get_decode_rate() << " msg/sec";
      }
      }
}

void manage_calls() {
  bool ended_call = false;
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

    if ((state == MONITORING) && (call->since_last_update() > config.call_timeout)) {
        ended_call = true;
        it = calls.erase(it);
        delete call;
        continue;
    }

    if (state == RECORDING)  {
      Recorder *recorder = call->get_recorder();

      // Stop the call if:
      // - there hasn't been an UPDATE for it on the Control Channel in X seconds AND the recorder hasn't written anything in X seconds
      // OR
      // - the recorder has been stopped
      if (((recorder->since_last_write() > config.call_timeout) /*&& (call->since_last_update() > config.call_timeout)*/) || (recorder->get_state() == STOPPED)) {
          BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[36m Stopping Call because of Recorder \u001b[0m Rec last write: " << recorder->since_last_write() << " State: " << format_state(recorder->get_state());

          call->set_state(COMPLETED);
          call->conclude_call();
          // The State of the Recorders has changed, so lets send an update
          ended_call = true;
          if (recorder != NULL) {
            plugman_setup_recorder(recorder);
          }
          it = calls.erase(it);
          delete call;
          continue;
    } else if (call->since_last_update() > config.call_timeout) {
          Recorder *recorder = call->get_recorder();
          // BOOST_LOG_TRIVIAL(info) << "Recorder state: " << recorder->get_state();
          BOOST_LOG_TRIVIAL(trace) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[36m  Call UPDATEs has been inactive for more than " << config.call_timeout << " Sec \u001b[0m Rec last write: " << recorder->since_last_write() << " State: " << format_state(recorder->get_state());

          // since the Call state is INACTIVE and the Recorder has been going on for a while, we can now
          // set the Call state to COMPLETED
         /* call->set_state(COMPLETED);
          call->conclude_call();
          // The State of the Recorders has changed, so lets send an update
          ended_call = true;

          if (recorder != NULL) {
            plugman_setup_recorder(recorder);
          }
          it = calls.erase(it);
          delete call;
          continue;*/
        }
    }

    ++it;
    // if rx is active
  } // foreach loggers

  if (ended_call) {
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

void unit_data_grant(System *sys, long source_id) {
  plugman_unit_data_grant(sys, source_id);
}

void unit_answer_request(System *sys, long source_id, long talkgroup) {
  plugman_unit_answer_request(sys, source_id, talkgroup);
}

void unit_location(System *sys, long source_id, long talkgroup_num) {
  plugman_unit_location(sys, source_id, talkgroup_num);
}

void handle_call_grant(TrunkMessage message, System *sys) {
  bool call_found = false;
  bool duplicate_grant = false;
  bool superseding_grant = false;
  bool recording_started [[maybe_unused]] = false;

  Call *original_call;

  /* Notes: it is possible for 2 Calls to exist for the same talkgroup on different freq. This happens when a Talkgroup starts on a freq
  that current recorder can't retune to. In this case, the current orig Talkgroup reocrder will keep going on the old freq, while a new
  recorder is start on a source that can cover that freq. This makes sure any of the remaining transmission that it is in the buffer
  of the original recorder gets flushed.
  UPDATED: however if we have 2 different talkgroups on the same freq we should do a stop_call on the original call since it is being used by another TG now. This will let the recorder keep
  going until it gets a termination flag.
  */

  //BOOST_LOG_TRIVIAL(info) << "TG: " << message.talkgroup << " sys num: " << message.sys_num << " freq: " << message.freq << " TDMA Slot" << message.tdma_slot << " TDMA: " << message.phase2_tdma;

  unsigned long message_preferredNAC = 0;
  Talkgroup *message_talkgroup = sys->find_talkgroup(message.talkgroup);
  if (message_talkgroup) {
     message_preferredNAC = message_talkgroup->get_preferredNAC();
  }

  for (vector<Call *>::iterator it = calls.begin(); it != calls.end();) {
    Call *call = *it;

    /* This will skip all calls that are not currently acitve */
    if (call->get_state() == COMPLETED) {
      ++it;
      continue;
    }

    if (call->get_talkgroup() == message.talkgroup) {
      if ((call->get_phase2_tdma() == message.phase2_tdma) && (call->get_tdma_slot() == message.tdma_slot) ) {
      if (call->get_sys_num() != message.sys_num) {
        if (call->get_system()->get_multiSite() && sys->get_multiSite()) {
          if (call->get_system()->get_wacn() == sys->get_wacn()) {
            // Default mode to match WACN and NAC and use a preferred NAC;
            if (call->get_system()->get_nac() != sys->get_nac() && (call->get_system()->get_multiSiteSystemName() == "")) {
              if (call->get_state() == RECORDING) {

                duplicate_grant = true;
                original_call = call;

                unsigned long call_preferredNAC = 0;
                Talkgroup *call_talkgroup = call->get_system()->find_talkgroup(message.talkgroup);
                if (call_talkgroup) {
                  call_preferredNAC = call_talkgroup->get_preferredNAC();
          
                }

                if ((call_preferredNAC != call->get_system()->get_nac() ) && (message_preferredNAC == sys->get_nac())) {
                  superseding_grant = true;
                }

              }
            }
            // If a multiSiteSystemName has been manually entered;
            // We already know that Call's system number does not match the message system number.
            // In this case, we check that the multiSiteSystemName is present, and that the Call and System multiSiteSystemNames are the same.
            else if ((call->get_system()->get_multiSiteSystemName() != "")  && (call->get_system()->get_multiSiteSystemName() == sys->get_multiSiteSystemName())) {
              if (call->get_state() == RECORDING) {

                duplicate_grant = true;
                original_call = call;

                unsigned long call_preferredNAC = 0;
                Talkgroup *call_talkgroup = call->get_system()->find_talkgroup(message.talkgroup);
                if (call_talkgroup) {
                  call_preferredNAC = call_talkgroup->get_preferredNAC();
                }

                if((call->get_system()->get_multiSiteSystemNumber() != 0 ) && (sys->get_multiSiteSystemNumber() != 0 ))
                {
                  if ((call_preferredNAC != call->get_system()->get_multiSiteSystemNumber()) && (message_preferredNAC == sys->get_multiSiteSystemNumber())) {
                    superseding_grant = true;
                  }
                }
              }
            }
          }
        }
        }
      }
    }

    if ((call->get_talkgroup() == message.talkgroup) && (call->get_sys_num() == message.sys_num) && (call->get_freq() == message.freq) && (call->get_tdma_slot() == message.tdma_slot) && (call->get_phase2_tdma() == message.phase2_tdma)) {
      call_found = true;
      bool source_updated = call->update(message);
      if (source_updated) {
        plugman_call_start(call);
      }
    }

    // There is an existing call on freq and slot that the new call will be started on. We should stop the older call. The older recorder will
    // keep writing to the file until it hits a termination flag, so no packets should be dropped.
    if ((call->get_state() == RECORDING) && (call->get_talkgroup() != message.talkgroup) && (call->get_sys_num() == message.sys_num) && (call->get_freq() == message.freq) && (call->get_tdma_slot() == message.tdma_slot) && (call->get_phase2_tdma() == message.phase2_tdma)) {
      Recorder *recorder = call->get_recorder();
      string recorder_state = "UNKNOWN";
      if (recorder != NULL) {
        recorder_state = format_state(recorder->get_state());
      }
      BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[36mShould be Stopping RECORDING call, Recorder State: " << recorder_state << " RX overlapping TG message Freq, TG:" << message.talkgroup << "\u001b[0m";
/*
      call->set_state(COMPLETED);
      call->conclude_call();
      it = calls.erase(it);
      delete call;
      continue;*/
    }

    it++;
  }

  if (!call_found) {
    Call *call = Call::make(message, sys, config);

    Talkgroup *talkgroup = sys->find_talkgroup(call->get_talkgroup());

    if (talkgroup) {
      call->set_talkgroup_tag(talkgroup->alpha_tag);
    } else {
      call->set_talkgroup_tag("-");
    }

    
    if(superseding_grant) {
      BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << original_call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[36mSuperseding Grant. Original Call NAC: " << original_call->get_system()->get_nac() << " Grant Message NAC: " << sys->get_nac() << "\t State: " << format_state(original_call->get_state()) << "\u001b[0m";

      // Attempt to start a new call on the preferred NAC.
      recording_started = start_recorder(call, message, sys);
      
      if(recording_started) {
        // Clean up the original call.
        original_call->set_state(MONITORING);
        original_call->set_monitoring_state(SUPERSEDED);
        original_call->conclude_call();
      }
      else{
        BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << original_call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[36mCould not start Superseding recorder. Continuing original call: " << original_call->get_call_num() << "C\u001b[0m";
      }
    }
    else if (duplicate_grant) {
      call->set_state(MONITORING);
      call->set_monitoring_state(DUPLICATE);
      BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << original_call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[36mDuplicate Grant. Original Call NAC: " << original_call->get_system()->get_nac() << " Grant Message NAC: " << sys->get_nac() << " Source: " << message.source << " Call: " << original_call->get_call_num() << "C State: " << format_state(original_call->get_state()) << "\u001b[0m";
    }
    else {
      recording_started = start_recorder(call, message, sys);
      if (message.message_type == UPDATE) {
        BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[36mThis was an UPDATE \u001b[0m";
      }
    }  
    calls.push_back(call);
    plugman_call_start(call);
    plugman_calls_active(calls);
  }
}

void handle_call_update(TrunkMessage message, System *sys) {
  bool call_found = false;

  /* Notes: it is possible for 2 Calls to exist for the same talkgroup on different freq. This happens when a Talkgroup starts on a freq
  that current recorder can't retune to. In this case, the current orig Talkgroup reocrder will keep going on the old freq, while a new
  recorder is start on a source that can cover that freq. This makes sure any of the remaining transmission that it is in the buffer
  of the original recorder gets flushed.
  UPDATED: however if we have 2 different talkgroups on the same freq we should do a stop_call on the original call since it is being used by another TG now. This will let the recorder keep
  going until it gets a termination flag.
  */

  for (vector<Call *>::iterator it = calls.begin(); it != calls.end(); ++it) {
    Call *call = *it;

    /* This will skip all calls that are not currently acitve */
    if (call->get_state() == COMPLETED) {
      continue;
    }

    // BOOST_LOG_TRIVIAL(info) << "TG: " << call->get_talkgroup() << " | " << message.talkgroup << " sys num: " << call->get_sys_num() << " | " << message.sys_num << " freq: " << call->get_freq() << " | " << message.freq << " TDMA Slot" << call->get_tdma_slot() << " | " << message.tdma_slot << " TDMA: " << call->get_phase2_tdma() << " | " << message.phase2_tdma;
    if ((call->get_talkgroup() == message.talkgroup) && (call->get_sys_num() == message.sys_num) && (call->get_freq() == message.freq) && (call->get_tdma_slot() == message.tdma_slot) && (call->get_phase2_tdma() == message.phase2_tdma)) {
      call_found = true;

      bool source_updated = call->update(message);
      if (source_updated) {
        plugman_call_start(call);
      }
    }
  }

  if (!call_found) {
    // Note: some calls maybe removed before the UPDATEs stop on the trunking channel if there is some GAP in the updates.
     BOOST_LOG_TRIVIAL(info) << "Call not found for UPDATE mesg - either we missed GRANT or removed Call too soon\tFreq: " << format_freq(message.freq) << "\tTG:" << message.talkgroup << "\tSource: " << message.source << "\tSys Num: " << message.sys_num << "\tTDMA Slot: " << message.tdma_slot << "\tTDMA: " << message.phase2_tdma;
  }
}

void handle_message(std::vector<TrunkMessage> messages, System *sys) {
  for (std::vector<TrunkMessage>::iterator it = messages.begin(); it != messages.end(); it++) {
    TrunkMessage message = *it;

    switch (message.message_type) {
    case GRANT:
      handle_call_grant(message, sys);
      break;

    case UPDATE:
      handle_call_update(message, sys);
      //handle_call_grant(message, sys);
      break;

    case UU_V_GRANT:
      if (config.record_uu_v_calls) {
        handle_call_grant(message, sys);
      }
      break;

    case UU_V_UPDATE:
      if (config.record_uu_v_calls) {
        handle_call_update(message, sys);
      }
      break;

    case CONTROL_CHANNEL:
      sys->add_control_channel(message.freq);
      break;

    case REGISTRATION:
      unit_registration(sys, message.source);
      break;

    case DEREGISTRATION:
      unit_deregistration(sys, message.source);
      break;

    case AFFILIATION:
      unit_group_affiliation(sys, message.source, message.talkgroup);
      break;

    case SYSID:
      break;

    case STATUS:
      current_system_status(message, sys);
      break;

    case LOCATION:
      unit_location(sys, message.source, message.talkgroup);
      break;

    case ACKNOWLEDGE:
      unit_acknowledge_response(sys, message.source);
      break;

    case PATCH_ADD:
      sys->update_active_talkgroup_patches(message.patch_data);
      break;
    case PATCH_DELETE:
      sys->delete_talkgroup_patch(message.patch_data);
      break;

    case DATA_GRANT:
      unit_data_grant(sys, message.source);
      break;

    case UU_ANS_REQ:
      unit_answer_request(sys, message.source, message.talkgroup);
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

void retune_system(System *sys) {
  System_impl *system = (System_impl *)sys;
  bool source_found = false;
  Source *current_source = system->get_source();
  double control_channel_freq = system->get_next_control_channel();

  BOOST_LOG_TRIVIAL(error) << "[" << system->get_short_name() << "] Retuning to Control Channel: " << format_freq(control_channel_freq);

  if ((current_source->get_min_hz() <= control_channel_freq) &&
      (current_source->get_max_hz() >= control_channel_freq)) {
    source_found = true;
    BOOST_LOG_TRIVIAL(info) << "\t - System Source " << current_source->get_num() << " - Min Freq: " << format_freq(current_source->get_min_hz()) << " Max Freq: " << format_freq(current_source->get_max_hz());
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
        BOOST_LOG_TRIVIAL(info) << "\t - System Source " << source->get_num() << " - Min Freq: " << format_freq(source->get_min_hz()) << " Max Freq: " << format_freq(source->get_max_hz());

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
    System_impl *sys = (System_impl *)*it;

    if ((sys->get_system_type() != "conventional") && (sys->get_system_type() != "conventionalP25") && (sys->get_system_type() != "conventionalDMR")) {
      int msgs_decoded_per_second = std::floor(sys->message_count / timeDiff);
      sys->set_decode_rate(msgs_decoded_per_second);

      if (msgs_decoded_per_second < 2) {

        // if it loses track of the control channel, quit after a while
        if (config.control_retune_limit > 0) {
          sys->retune_attempts++;
          if (sys->retune_attempts > config.control_retune_limit) {
            BOOST_LOG_TRIVIAL(error) << "[" << sys->get_short_name() << "]\t" << "Control channel retune limit exceeded after " << sys->retune_attempts << " tries - Terminating trunk recorder";
            exit_flag = 1;
            exit_code = EXIT_FAILURE;
            return;
          }
        }
        if (sys->control_channel_count() > 1) {
          retune_system(sys);
        } else {
          BOOST_LOG_TRIVIAL(error) << "[" << sys->get_short_name() << "]\tThere is only one control channel defined";
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

  time_t last_status_time = time(NULL);
  time_t last_decode_rate_check = time(NULL);
  time_t management_timestamp = time(NULL);
  time_t current_time = time(NULL);
  std::vector<TrunkMessage> trunk_messages;

  while (1) {

    if (exit_flag) { // my action when signal set it 1
      BOOST_LOG_TRIVIAL(info) << "Caught an Exit Signal...";
      for (vector<Call *>::iterator it = calls.begin(); it != calls.end();) {
        Call *call = *it;

        if (call->get_state() != MONITORING) {
          call->set_state(COMPLETED);
          call->conclude_call();
        }

        it = calls.erase(it);
        delete call;
      }

      BOOST_LOG_TRIVIAL(info) << "Cleaning up & Exiting...";

      // Sleep for 5 seconds to allow for all of the Call Concluder threads to finish.
      boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
      return;
    }

    process_message_queues();

    plugman_poll_one();


for (vector<System *>::iterator sys_it = systems.begin(); sys_it != systems.end(); sys_it++) {
    System_impl *system = (System_impl *)*sys_it;


    if ((system->get_system_type() == "p25") || (system->get_system_type() == "smartnet") ) {
      msg.reset();
      msg = system->get_msg_queue()->delete_head_nowait();
    while (msg != 0) {
        system->set_message_count(system->get_message_count() + 1);

        if (system->get_system_type() == "smartnet") {
          trunk_messages = smartnet_parser->parse_message(msg->to_string(), system);
          handle_message(trunk_messages, system);
        }

        if (system->get_system_type() == "p25") {
          trunk_messages = p25_parser->parse_message(msg, system);
          handle_message(trunk_messages, system);
        }
      

      if (msg->type() == -1) {
        BOOST_LOG_TRIVIAL(error) << "[" << system->get_short_name() << "]\t process_data_unit timeout";
      }

      msg.reset();
      msg = system->get_msg_queue()->delete_head_nowait();
    } 
    }
}
      current_time = time(NULL);

      if ((current_time - management_timestamp) >= 1.0) {
        manage_calls();
        Call_Concluder::manage_call_data_workers();
        management_timestamp = current_time;
      }

      boost::this_thread::sleep(boost::posix_time::milliseconds(10));
      // usleep(1000 * 10);
    

    float decode_rate_check_time_diff = current_time - last_decode_rate_check;

    if (decode_rate_check_time_diff >= 3.0) {
      check_message_count(decode_rate_check_time_diff);
      last_decode_rate_check = current_time;
      for (vector<System *>::iterator sys_it = systems.begin(); sys_it != systems.end(); sys_it++) {
        System *system = *sys_it;
        if (system->get_system_type() == "p25") {
          system->clear_stale_talkgroup_patches();
        }
      }
    }

    float print_status_time_diff = current_time - last_status_time;

    if (print_status_time_diff > 200) {
      last_status_time = current_time;
      print_status();
    }
  }
}

bool setup_convetional_channel(System *system, double frequency, long channel_index) {
  bool channel_added = false;
  Source *source = NULL;
  for (vector<Source *>::iterator src_it = sources.begin(); src_it != sources.end(); src_it++) {
    source = *src_it;

    if ((source->get_min_hz() <= frequency) && (source->get_max_hz() >= frequency)) {
      channel_added = true;
      if (system->get_squelch_db() == -160) {
        BOOST_LOG_TRIVIAL(error) << "[" << system->get_short_name() << "]\tSquelch needs to be specified for the Source for Conventional Systems";
        return false;
      } else {
        channel_added = true;
      }

      Call_conventional *call = NULL;
      if (system->has_channel_file()) {
        Talkgroup *tg = system->find_talkgroup_by_freq(frequency);
        call = new Call_conventional(tg->number, tg->freq, system, config);
        call->set_talkgroup_tag(tg->alpha_tag);
      } else {
        call = new Call_conventional(channel_index, frequency, system, config);
      }
      BOOST_LOG_TRIVIAL(info) << "[" << system->get_short_name() << "]\tMonitoring " << system->get_system_type() << " channel: " << format_freq(frequency) << " Talkgroup: " << channel_index;
      if (system->get_system_type() == "conventional") {
        analog_recorder_sptr rec;
        rec = source->create_conventional_recorder(tb);
        rec->start(call);
        call->set_is_analog(true);
        call->set_recorder((Recorder *)rec.get());
        call->set_state(RECORDING);
        system->add_conventional_recorder(rec);
        calls.push_back(call);
        plugman_setup_recorder((Recorder *)rec.get());
        plugman_call_start(call);
      } else if (system->get_system_type() == "conventionalDMR") {
        // Because of dynamic mod assignment we can not start the recorder until the graph has been unlocked.
        // This has something to do with the way the Selector block works.
        // the manage_conventional_calls() function handles adding and starting the P25 Recorder
        dmr_recorder_sptr rec;
        rec = source->create_dmr_conventional_recorder(tb);
        call->set_recorder((Recorder *)rec.get());
        system->add_conventionalDMR_recorder(rec);
        calls.push_back(call);
      } else { // has to be "conventional P25"
        // Because of dynamic mod assignment we can not start the recorder until the graph has been unlocked.
        // This has something to do with the way the Selector block works.
        // the manage_conventional_calls() function handles adding and starting the P25 Recorder
        p25_recorder_sptr rec;
        rec = source->create_digital_conventional_recorder(tb);
        call->set_recorder((Recorder *)rec.get());
        system->add_conventionalP25_recorder(rec);
        calls.push_back(call);
      }

      // break out of the for loop
      break;
    }
  }
  return channel_added;
}

bool setup_conventional_system(System *system) {
  bool system_added = false;

  if (system->has_channel_file()) {
    std::vector<Talkgroup *> talkgroups = system->get_talkgroups();
    for (vector<Talkgroup *>::iterator tg_it = talkgroups.begin(); tg_it != talkgroups.end(); tg_it++) {
      Talkgroup *tg = *tg_it;

      bool channel_added = setup_convetional_channel(system, tg->freq, tg->number);

      if (!channel_added) {
        BOOST_LOG_TRIVIAL(error) << "[" << system->get_short_name() << "]\t Unable to find a source for this conventional channel! Channel not added: " << format_freq(tg->freq) << " Talkgroup: " << tg->number;
        // return false;
      } else {
        system_added = true;
      }
    }
  } else {
    std::vector<double> channels = system->get_channels();
    int channel_index = 0;
    for (vector<double>::iterator chan_it = channels.begin(); chan_it != channels.end(); chan_it++) {
      double channel = *chan_it;
      ++channel_index;
      bool channel_added = setup_convetional_channel(system, channel, channel_index);

      if (!channel_added) {
        BOOST_LOG_TRIVIAL(error) << "[" << system->get_short_name() << "]\t Unable to find a source for this conventional channel! Channel not added: " << format_freq(channel) << " Talkgroup: " << channel_index;
        // return false;
      } else {
        system_added = true;
      }
    }
  }
  return system_added;
}

bool setup_systems() {

  Source *source = NULL;

  for (vector<System *>::iterator sys_it = systems.begin(); sys_it != systems.end(); sys_it++) {
    System_impl *system = (System_impl *)*sys_it;
    // bool    source_found = false;
    bool system_added = false;
    if ((system->get_system_type() == "conventional") || (system->get_system_type() == "conventionalP25") || (system->get_system_type() == "conventionalDMR")) {
      system_added = setup_conventional_system(system);
    } else {
      // If it's not a conventional system, then it's a trunking system
      double control_channel_freq = system->get_current_control_channel();
      BOOST_LOG_TRIVIAL(info) << "[" << system->get_short_name() << "]\tStarted with Control Channel: " << format_freq(control_channel_freq);

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
                                                               system->get_msg_queue(),
                                                               system->get_sys_num());
            tb->connect(source->get_src_block(), 0, system->smartnet_trunking, 0);
          }

          if (system->get_system_type() == "p25") {
            // what you really need to do is go through all of the sources to find
            // the one with the right frequencies
            system->p25_trunking = make_p25_trunking(control_channel_freq,
                                                     source->get_center(),
                                                     source->get_rate(),
                                                     system->get_msg_queue(),
                                                     system->get_qpsk_mod(),
                                                     system->get_sys_num());
            tb->connect(source->get_src_block(), 0, system->p25_trunking, 0);
          }

          // break out of the For Loop
          break;
        }
      }
      if (!system_added) {
        BOOST_LOG_TRIVIAL(error) << "[" << system->get_short_name() << "]\t Unable to find a source for this System! Control Channel Freq: " << format_freq(control_channel_freq);
        return false;
      }
    }
  }
  return true;
}

template <class F>
void add_logs(const F &fmt) {
  boost::shared_ptr<sinks::synchronous_sink<sinks::basic_text_ostream_backend<char>>> sink =
      boost::log::add_console_log(std::clog, boost::log::keywords::format = fmt);

  std::locale loc = std::locale("C");

  sink->imbue(loc);
}

int main(int argc, char **argv) {
  // BOOST_STATIC_ASSERT(true) __attribute__((unused));

  logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::info);

  boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");

  boost::log::add_common_attributes();
  boost::log::core::get()->add_global_attribute("Scope", boost::log::attributes::named_scope());
  boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);

  add_logs(boost::log::expressions::format("[%1%] (%2%)   %3%") % boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f") % boost::log::expressions::attr<boost::log::trivial::severity_level>("Severity") % boost::log::expressions::smessage);

  // boost::log::sinks->imbue(std::locale("C"));
  // std::locale::global(std::locale("C"));

  boost::program_options::options_description desc("Options");
  desc.add_options()("help,h", "Help screen")("config,c", boost::program_options::value<string>()->default_value("./config.json"), "Config File")("version,v", "Version Information");

  boost::program_options::variables_map vm;
  boost::program_options::store(parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);

  if (vm.count("version")) {
    GitMetadata::VersionInfo();
    exit(0);
  }

  if (vm.count("help")) {
    std::cout << "Usage: trunk-recorder [options]\n";
    std::cout << desc;
    exit(0);
  }
  string config_file = vm["config"].as<string>();

  if (vm.count("config")) {
    BOOST_LOG_TRIVIAL(info) << "Using Config file: " << config_file << "\n";
  }
  BOOST_LOG_TRIVIAL(info) << PROJECT_NAME << ": "
                          << "Version: " << PROJECT_VER << "\n";

  tb = gr::make_top_block("Trunking");
  tb->start();
  tb->lock();
  
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
        keywords::rotation_size = 100 * 1024 * 1024,
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

  return exit_code;
}
