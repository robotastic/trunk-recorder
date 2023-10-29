/**
 * Method name: load_config()
 * Description: <#description#>
 * Parameters: <#parameters#>
 */
#include "./config.h"


namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;

using namespace std;


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

template <class F>
void add_logs(const F &fmt) {
  boost::shared_ptr<sinks::synchronous_sink<sinks::basic_text_ostream_backend<char>>> sink =
      boost::log::add_console_log(std::clog, boost::log::keywords::format = fmt);

  std::locale loc = std::locale("C");

  sink->imbue(loc);
}

bool load_config(string config_file, Config& config, gr::top_block_sptr& tb, std::vector<Source *>& sources, std::vector<System *>& systems) {
  
  string system_modulation;
  int sys_count = 0;
  int source_count = 0;

  try {
    // const std::string json_filename = "config.json";

    boost::property_tree::ptree pt;
    boost::property_tree::read_json(config_file, pt);

    config.console_log = pt.get<bool>("consoleLog", true);
    if (config.console_log) {
      add_logs(boost::log::expressions::format("[%1%] (%2%)   %3%") % boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f") % boost::log::expressions::attr<boost::log::trivial::severity_level>("Severity") % boost::log::expressions::smessage);
    }

    BOOST_LOG_TRIVIAL(info) << "\n-------------------------------------\n     Trunk Recorder\n-------------------------------------\n";

    BOOST_LOG_TRIVIAL(info) << "\n\n-------------------------------------\nINSTANCE\n-------------------------------------\n";

    config.log_file = pt.get<bool>("logFile", false);
    config.log_dir = pt.get<std::string>("logDir", "logs");
    if (config.log_file) {
      logging::add_file_log(
          keywords::file_name = config.log_dir + "/%m-%d-%Y_%H%M_%2N.log",
          keywords::format = "[%TimeStamp%] (%Severity%)   %Message%",
          keywords::rotation_size = 100 * 1024 * 1024,
          keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
          keywords::auto_flush = true);
    }

    double config_ver = pt.get<double>("ver", 0);
    if (config_ver < 2) {
      BOOST_LOG_TRIVIAL(info) << "The formatting for config files has changed.";
      BOOST_LOG_TRIVIAL(info) << "Modulation type, Squelch and audio levels are now set in each System instead of under a Source.";
      BOOST_LOG_TRIVIAL(info) << "See sample config files in the /example folder and look at readme.md for more details.";
      BOOST_LOG_TRIVIAL(info) << "After you have made these updates, make sure you add \"ver\": 2, to the top.\n\n";
      return false;
    }

    BOOST_LOG_TRIVIAL(info) << "Using Config file: " << config_file << "\n";
    BOOST_LOG_TRIVIAL(info) << PROJECT_NAME << ": "
                            << "Version: " << PROJECT_VER << "\n";

    BOOST_LOG_TRIVIAL(info) << "Log to File: " << config.log_file;
    BOOST_LOG_TRIVIAL(info) << "Log Directory: " << config.log_dir;

    std::string defaultTempDir = boost::filesystem::current_path().string();

    if (boost::filesystem::exists("/dev/shm")) {
      defaultTempDir = "/dev/shm";
    }
    config.temp_dir = pt.get<std::string>("tempDir", defaultTempDir);
    size_t pos = config.temp_dir.find_last_of("/");

    if (pos == config.temp_dir.length() - 1) {
      config.temp_dir.erase(config.temp_dir.length() - 1);
    }

    BOOST_LOG_TRIVIAL(info) << "Temporary Transmission Directory: " << config.temp_dir;

    config.capture_dir = pt.get<std::string>("captureDir", boost::filesystem::current_path().string());
    pos = config.capture_dir.find_last_of("/");

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
    config.default_mode = pt.get<std::string>("defaultMode", "digital");
    BOOST_LOG_TRIVIAL(info) << "Default Mode: " << config.default_mode;
    config.call_timeout = pt.get<int>("callTimeout", 3);
    BOOST_LOG_TRIVIAL(info) << "Call Timeout (seconds): " << config.call_timeout;
    config.control_message_warn_rate = pt.get<int>("controlWarnRate", 10);
    BOOST_LOG_TRIVIAL(info) << "Control channel warning rate: " << config.control_message_warn_rate;
    config.control_retune_limit = pt.get<int>("controlRetuneLimit", 0);
    BOOST_LOG_TRIVIAL(info) << "Control channel retune limit: " << config.control_retune_limit;
    config.soft_vocoder = pt.get<bool>("softVocoder", false);
    BOOST_LOG_TRIVIAL(info) << "Phase 1 Software Vocoder: " << config.soft_vocoder;
    config.enable_audio_streaming = pt.get<bool>("audioStreaming", false);
    BOOST_LOG_TRIVIAL(info) << "Enable Audio Streaming: " << config.enable_audio_streaming;
    config.record_uu_v_calls = pt.get<bool>("recordUUVCalls", true);
    BOOST_LOG_TRIVIAL(info) << "Record Unit to Unit Voice Calls: " << config.record_uu_v_calls;
    config.new_call_from_update = pt.get<bool>("newCallFromUpdate", true);
    BOOST_LOG_TRIVIAL(info) << "New Call from UPDATE Messages" << config.new_call_from_update;
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
            system->add_control_channel(control_channel);
          }
          std::vector<double> control_channels = system->get_control_channels();
          for (unsigned int i = 0; i < control_channels.size(); i++) {
            BOOST_LOG_TRIVIAL(info) << "  " << format_freq(control_channels[i]);
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