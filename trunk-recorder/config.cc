#include "config.h"
#include "formatter.h"
/**
 * Method name: load_config()
 * Description: <#description#>
 * Parameters: <#parameters#>
 */
Config load_config(std::string config_file, std::vector<Source *> &sources, std::vector<System *> &systems)
{
  std::string system_modulation;
  Config config;
  int    sys_count = 0;

  try
  {
    // const std::string json_filename = "config.json";

    boost::property_tree::ptree pt;
    boost::property_tree::read_json(config_file, pt);
    BOOST_FOREACH(boost::property_tree::ptree::value_type  & node,
                  pt.get_child("systems"))
    {
      // each system should have a unique index value;
      System *system = new System(sys_count++);
      std::stringstream default_script;

      default_script << "sys_" << sys_count;

      system->set_system_type(node.second.get<std::string>("type"));
      BOOST_LOG_TRIVIAL(info) << "System Type: " << system->get_system_type();

      if (system->get_system_type() == "conventional") {
        BOOST_LOG_TRIVIAL(info) << "Conventional Channels: ";
        BOOST_FOREACH(boost::property_tree::ptree::value_type  & sub_node, node.second.get_child("channels"))
        {
          double channel = sub_node.second.get<double>("", 0);

          BOOST_LOG_TRIVIAL(info) << sub_node.second.get<double>("", 0) << " ";
          system->add_channel(channel);
        }
      }  else if (system->get_system_type() == "conventionalP25") {
        BOOST_LOG_TRIVIAL(info) << "Conventional Channels: ";
        BOOST_FOREACH(boost::property_tree::ptree::value_type  & sub_node, node.second.get_child("channels"))
        {
          double channel = sub_node.second.get<double>("", 0);

          BOOST_LOG_TRIVIAL(info) << sub_node.second.get<double>("", 0) << " ";
          system->add_channel(channel);
        }
      } else if ((system->get_system_type() == "smartnet") || (system->get_system_type() == "p25")) {
        BOOST_LOG_TRIVIAL(info) << "Control Channels: ";
        BOOST_FOREACH(boost::property_tree::ptree::value_type  & sub_node, node.second.get_child("control_channels"))
        {
          double control_channel = sub_node.second.get<double>("", 0);

          BOOST_LOG_TRIVIAL(info) << sub_node.second.get<double>("", 0) << " ";
          system->add_control_channel(control_channel);
        }
      } else {
        BOOST_LOG_TRIVIAL(error) << "System Type in config.json not recognized";
        exit(1);
      }
      BOOST_LOG_TRIVIAL(info) << "\n-------------------------------------\nSystem Number: " << sys_count;
      system->set_short_name(node.second.get<std::string>("shortName", default_script.str()));
      BOOST_LOG_TRIVIAL(info) << "Short Name: " << system->get_short_name();

      system->set_api_key(node.second.get<std::string>("apiKey", ""));
      BOOST_LOG_TRIVIAL(info) << "API Key: " << system->get_api_key();

      system->set_upload_script(node.second.get<std::string>("uploadScript", ""));
      BOOST_LOG_TRIVIAL(info) << "Upload Script: " << config.upload_script;
      system->set_call_log(node.second.get<bool>("callLog", true));
      BOOST_LOG_TRIVIAL(info) << "Call Log: " << system->get_call_log();
      system->set_audio_archive(node.second.get<bool>("audioArchive", true));
      BOOST_LOG_TRIVIAL(info) << "Audio Archive: " << system->get_audio_archive();
      system->set_talkgroups_file(node.second.get<std::string>("talkgroupsFile", ""));
      BOOST_LOG_TRIVIAL(info) << "Talkgroups File: " << system->get_talkgroups_file();
      system->set_record_unknown(node.second.get<bool>("recordUnknown", true));
      BOOST_LOG_TRIVIAL(info) << "Record Unknown Talkgroups: " << system->get_record_unknown();
      system->set_min_duration(node.second.get<double>("minDuration", 0));
      BOOST_LOG_TRIVIAL(info) << "Minimum Call Duration (in seconds): " << system->get_min_duration();
      systems.push_back(system);

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
      BOOST_LOG_TRIVIAL(info);
    }

    config.capture_dir = pt.get<std::string>("captureDir", boost::filesystem::current_path().string());
    size_t pos = config.capture_dir.find_last_of("/");

    if (pos == config.capture_dir.length() - 1)
    {
      config.capture_dir.erase(config.capture_dir.length() - 1);
    }
    BOOST_LOG_TRIVIAL(info) << "Capture Directory: " << config.capture_dir;
    config.upload_server = pt.get<std::string>("uploadServer", "encode-upload.sh");
    BOOST_LOG_TRIVIAL(info) << "Upload Server: " << config.upload_server;
    default_mode = pt.get<std::string>("defaultMode", "digital");
    BOOST_LOG_TRIVIAL(info) << "Default Mode: " << default_mode;
    config.call_timeout = pt.get<int>("callTimeout", 3);
    BOOST_LOG_TRIVIAL(info) << "Call Timeout (seconds): " << config.call_timeout;
    config.log_file = pt.get<bool>("logFile", false);
    BOOST_LOG_TRIVIAL(info) << "Log to File: " << config.log_file;
    config.control_message_warn_rate = pt.get<int>("controlWarnRate", 10);
    BOOST_LOG_TRIVIAL(info) << "Control channel rate warning: " << config.control_message_warn_rate;


    BOOST_FOREACH(boost::property_tree::ptree::value_type  & node,
                  pt.get_child("sources"))
    {
      bool   qpsk_mod       = true;
      int    silence_frames = node.second.get<int>("silenceFrames", 0);
      double center         = node.second.get<double>("center", 0);
      double rate           = node.second.get<double>("rate", 0);
      double error          = node.second.get<double>("error", 0);
      double ppm            = node.second.get<double>("ppm", 0);
      int    gain           = node.second.get<double>("gain", 0);
      int    if_gain        = node.second.get<double>("ifGain", 0);
      int    bb_gain        = node.second.get<double>("bbGain", 0);
      int    mix_gain       = node.second.get<double>("mixGain", 0);
      int    lna_gain       = node.second.get<double>("lnaGain", 0);
      int    pga_gain       = node.second.get<double>("pgaGain", 0);
      int    tia_gain       = node.second.get<double>("tiaGain", 0);
      int    vga1_gain      = node.second.get<double>("vga1Gain", 0);
      int    vga2_gain      = node.second.get<double>("vga2Gain", 0);
      double fsk_gain       = node.second.get<double>("fskGain", 1.0);
      double digital_levels = node.second.get<double>("digitalLevels", 8.0);
      double analog_levels  = node.second.get<double>("analogLevels", 8.0);
      double squelch_db     = node.second.get<double>("squelch", 0);
      std::string antenna   = node.second.get<std::string>("antenna", "");
      int digital_recorders = node.second.get<int>("digitalRecorders", 0);
      int debug_recorders   = node.second.get<int>("debugRecorders", 0);
      int analog_recorders  = node.second.get<int>("analogRecorders", 0);

      std::string driver = node.second.get<std::string>("driver", "");

      if ((driver != "osmosdr") && (driver != "usrp")) {
        BOOST_LOG_TRIVIAL(error) << "Driver specified in config.json not recognized, needs to be osmosdr or usrp";
      }

      std::string device = node.second.get<std::string>("device", "");

      BOOST_LOG_TRIVIAL(info) << "Center: " << node.second.get<double>("center", 0);
      BOOST_LOG_TRIVIAL(info) << "Rate: " << node.second.get<double>("rate", 0);
      BOOST_LOG_TRIVIAL(info) << "Error: " << node.second.get<double>("error", 0);
      BOOST_LOG_TRIVIAL(info) << "PPM Error: " <<  node.second.get<double>("ppm", 0);
      BOOST_LOG_TRIVIAL(info) << "Gain: " << node.second.get<double>("gain", 0);
      BOOST_LOG_TRIVIAL(info) << "IF Gain: " << node.second.get<double>("ifGain", 0);
      BOOST_LOG_TRIVIAL(info) << "BB Gain: " << node.second.get<double>("bbGain", 0);
      BOOST_LOG_TRIVIAL(info) << "LNA Gain: " << node.second.get<double>("lnaGain", 0);
      BOOST_LOG_TRIVIAL(info) << "PGA Gain: " << node.second.get<double>("pgaGain", 0);
      BOOST_LOG_TRIVIAL(info) << "TIA Gain: " << node.second.get<double>("tiaGain", 0);
      BOOST_LOG_TRIVIAL(info) << "MIX Gain: " << node.second.get<double>("mixGain", 0);
      BOOST_LOG_TRIVIAL(info) << "VGA1 Gain: " << node.second.get<double>("vga1Gain", 0);
      BOOST_LOG_TRIVIAL(info) << "VGA2 Gain: " << node.second.get<double>("vga2Gain", 0);
      BOOST_LOG_TRIVIAL(info) << "Squelch: " << node.second.get<double>("squelch", 0);
      BOOST_LOG_TRIVIAL(info) << "Idle Silence: " << node.second.get<bool>("idleSilence", 0);
      BOOST_LOG_TRIVIAL(info) << "Digital Recorders: " << node.second.get<int>("digitalRecorders", 0);
      BOOST_LOG_TRIVIAL(info) << "Debug Recorders: " << node.second.get<int>("debugRecorders",  0);
      BOOST_LOG_TRIVIAL(info) << "Analog Recorders: " << node.second.get<int>("analogRecorders",  0);
      BOOST_LOG_TRIVIAL(info) << "Driver: " << node.second.get<std::string>("driver",  "");

      boost::optional<std::string> mod_exists = node.second.get_optional<std::string>("modulation");

      if (mod_exists) {
        system_modulation = node.second.get<std::string>("modulation");

        if (boost::iequals(system_modulation, "qpsk"))
        {
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

      if ((ppm != 0) && (error != 0)) {
        BOOST_LOG_TRIVIAL(info) <<  "Both PPM and Error should not be set at the same time. Setting Error to 0.";
        error = 0;
      }

      Source *source = new Source(center, rate, error, driver, device, &config);
      BOOST_LOG_TRIVIAL(info) << "Max HZ: " << FormatFreqHz(source->get_max_hz());
      BOOST_LOG_TRIVIAL(info) << "Min HZ: " << FormatFreqHz(source->get_min_hz());

      if (if_gain != 0) {
        source->set_if_gain(if_gain);
      }

      if (bb_gain != 0) {
        source->set_bb_gain(bb_gain);
      }

      if (mix_gain != 0) {
        source->set_mix_gain(mix_gain);
      }

      source->set_lna_gain(lna_gain);

      source->set_tia_gain(tia_gain);

      source->set_pga_gain(pga_gain);

      if (vga1_gain != 0) {
        source->set_vga1_gain(vga1_gain);
      }

      if (vga2_gain != 0) {
        source->set_vga2_gain(vga2_gain);
      }

      source->set_gain(gain);
      source->set_antenna(antenna);
      source->set_squelch_db(squelch_db);
      source->set_fsk_gain(fsk_gain);
      source->set_analog_levels(analog_levels);
      source->set_digital_levels(digital_levels);
      source->set_qpsk_mod(qpsk_mod);
      source->set_silence_frames(silence_frames);

      if (ppm != 0) {
        source->set_freq_corr(ppm);
      }
      source->create_digital_recorders(tb, digital_recorders);
      source->create_analog_recorders(tb, analog_recorders);
      source->create_debug_recorders(tb, debug_recorders);
      sources.push_back(source);
    }
  }
  catch (std::exception const& e)
  {
    BOOST_LOG_TRIVIAL(error) << "Failed parsing Config: " << e.what();
  }
  return config;
}
