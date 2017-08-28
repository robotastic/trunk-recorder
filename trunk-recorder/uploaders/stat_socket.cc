#include "stat_socket.h"

/**
 * The telemetry client connects to a WebSocket server and sends a message every
 * second containing an integer count. This example can be used as the basis for
 * programs where a client connects and pushes data for logging, stress/load
 * testing, etc.
 */
void stat_socket::send_sys_rates(std::vector<System *>systems, float timeDiff) {
  boost::property_tree::ptree root;
  boost::property_tree::ptree systems_node;

  for (std::vector<System *>::iterator it = systems.begin(); it != systems.end(); ++it) {
    System *sys = (System *)*it;

    if ((sys->system_type != "conventional") && (sys->system_type != "conventionalP25")) {
      float msgs_decoded_per_second = sys->message_count / timeDiff;

      // Create a node
      boost::property_tree::ptree sys_node;

      // Add animals as childs
      systems_node.put<double>(sys->short_name, (float)msgs_decoded_per_second);

      // sys_node.add_child("shortName", sys->short_name);
      // sys_node.add_child("messages", msgs_decoded_per_second);
      // Add the new node to the root

      //systems_node.add_child()(sys->short_name, (float)msgs_decoded_per_second));
      //systems_node.push_back(std::make_pair("", sys_node));
    }
  }
  root.put("type", "rate");
  root.add_child("rates", systems_node);
  std::stringstream stats_str;
  boost::property_tree::write_json(stats_str, root);

  // std::cout << stats_str;
  send_stat(stats_str.str());
}

stat_socket::stat_socket() : m_open(false), m_done(false) {
  // set up access channels to only log interesting things
  m_client.clear_access_channels(websocketpp::log::alevel::all);
  m_client.set_access_channels(websocketpp::log::alevel::connect);
  m_client.set_access_channels(websocketpp::log::alevel::disconnect);
  m_client.set_access_channels(websocketpp::log::alevel::app);

  // Initialize the Asio transport policy
  m_client.init_asio();

  // Bind the handlers we are using
  using websocketpp::lib::placeholders::_1;
  using websocketpp::lib::bind;
  m_client.set_open_handler(bind(&stat_socket::on_open, this, _1));
  m_client.set_close_handler(bind(&stat_socket::on_close, this, _1));
  m_client.set_fail_handler(bind(&stat_socket::on_fail, this, _1));
}

void stat_socket::send_config(std::vector<Source *>sources, std::vector<System *>systems, Config config) {
  boost::property_tree::ptree root;
  boost::property_tree::ptree systems_node;
  boost::property_tree::ptree sources_node;

  for (std::vector<Source *>::iterator it = sources.begin(); it != sources.end(); it++) {
    Source *source = *it;
    boost::property_tree::ptree source_node;

    source_node.put("antenna",           source->get_antenna());
    source_node.put("qpsk",              source->get_qpsk_mod());
    source_node.put("silence_frames",    source->get_silence_frames());
    source_node.put("analog_levels",     source->get_analog_levels());
    source_node.put("digital_levels",    source->get_digital_levels());
    source_node.put("min_hz",            source->get_min_hz());
    source_node.put("max_hz",            source->get_max_hz());
    source_node.put("center",            source->get_center());
    source_node.put("rate",              source->get_rate());
    source_node.put("driver",            source->get_driver());
    source_node.put("device",            source->get_device());
    source_node.put("error",             source->get_error());
    source_node.put("mix_gain",          source->get_mix_gain());
    source_node.put("lna_gain",          source->get_lna_gain());
    source_node.put("bb_gain",           source->get_bb_gain());
    source_node.put("gain",              source->get_gain());
    source_node.put("if_gain",           source->get_if_gain());
    source_node.put("squelch_db",        source->get_squelch_db());
    source_node.put("antenna",           source->get_antenna());
    source_node.put("analog_recorders",  source->analog_recorder_count());
    source_node.put("digital_recorders", source->digital_recorder_count());
    source_node.put("debug_recorders",   source->debug_recorder_count());

    sources_node.push_back(std::make_pair("", source_node));
  }

  for (std::vector<System *>::iterator it = systems.begin(); it != systems.end(); ++it) {
    System *sys = (System *)*it;

    boost::property_tree::ptree sys_node;
    boost::property_tree::ptree channels_node;
    sys_node.put("audioArchive",   sys->get_audio_archive());
    sys_node.put("systemType",     sys->get_system_type());
    sys_node.put("shortName",      sys->get_short_name());
    sys_node.put("sysNum",         sys->get_sys_num());
    sys_node.put("uploadScript",   sys->get_upload_script());
    sys_node.put("recordUnkown",   sys->get_record_unknown());
    sys_node.put("callLog",        sys->get_call_log());
    sys_node.put("talkgroupsFile", sys->get_talkgroups_file());
    std::vector<double> channels;

    if ((sys->get_system_type() == "conventional") || (sys->get_system_type() == "conventionalP25")) {
      channels = sys->get_channels();
    } else {
      channels = sys->get_control_channels();
    }

    std::cout << "starts: " << std::endl;

    for (std::vector<double>::iterator chan_it = channels.begin(); chan_it != channels.end(); chan_it++) {
      double channel = *chan_it;
      boost::property_tree::ptree channel_node;
      std::cout << "Hello: " << channel << std::endl;
      channel_node.put("", channel);

      // Add this node to the list.
      channels_node.push_back(std::make_pair("", channel_node));
    }
    sys_node.add_child("channels", channels_node);

    if (sys->system_type == "smartnet") {
      sys_node.put("bandplan",         sys->get_bandplan());
      sys_node.put("bandfreq",         sys->get_bandfreq());
      sys_node.put("bandplan_base",    sys->get_bandplan_base());
      sys_node.put("bandplan_high",    sys->get_bandplan_high());
      sys_node.put("bandplan_spacing", sys->get_bandplan_spacing());
      sys_node.put("bandplan_offset",  sys->get_bandplan_offset());
    }
    systems_node.push_back(std::make_pair("", sys_node));
  }
  root.add_child("sources", sources_node);
  root.add_child("systems", systems_node);
  root.put("captureDir",   config.capture_dir);
  root.put("uploadServer", config.upload_server);

  // root.put("defaultMode", default_mode);
  root.put("callTimeout",  config.call_timeout);
  root.put("logFile",      config.log_file);
  root.put("type",         "config");
  std::stringstream stats_str;
  boost::property_tree::write_json(stats_str, root);

  // std::cout << stats_str;
  send_stat(stats_str.str());
}

void stat_socket::send_status(std::vector<Call *>calls) {
  boost::property_tree::ptree root;
  boost::property_tree::ptree calls_node;
  boost::property_tree::ptree sources_node;

  for (std::vector<Call *>::iterator it = calls.begin(); it != calls.end(); it++) {
    Call *call         = *it;
    Recorder *recorder = call->get_recorder();

    boost::property_tree::ptree call_node;
    boost::property_tree::ptree freq_list_node;
    call_node.put("freq",         call->get_freq());
    call_node.put("sysNum",       call->get_sys_num());
    call_node.put("shortName",    call->get_short_name());
    call_node.put("talkgroup",    call->get_talkgroup());
    call_node.put("elasped",      call->elapsed());
    call_node.put("length",       call->get_current_length());
    call_node.put("state",        call->get_state());
    call_node.put("phase2",       call->get_phase2_tdma());
    call_node.put("conventional", call->is_conventional());
    call_node.put("encrypted",    call->get_encrypted());
    call_node.put("emergency",    call->get_emergency());
    call_node.put("startTime",    call->get_start_time());

    Call_Freq *freq_list = call->get_freq_list();
    int freq_count       = call->get_freq_count();

    for (int i = 0; i < freq_count; i++) {
      boost::property_tree::ptree freq_node;

      freq_node.put("freq", freq_list[i].freq);
      freq_node.put("time", freq_list[i].time);
      freq_list_node.push_back(std::make_pair("", freq_node));
    }
    call_node.add_child("freqList", freq_list_node);


    if (recorder) {
      Source *source = recorder->get_source();
      call_node.put("recNum",   recorder->get_num());
      call_node.put("srcNum",   source->get_num());
      call_node.put("recState", recorder->get_state());
    }
    calls_node.push_back(std::make_pair("", call_node));
  }
  root.add_child("systems", calls_node);
  root.put("type", "status");
  std::stringstream stats_str;
  boost::property_tree::write_json(stats_str, root);

  // std::cout << stats_str;
  send_stat(stats_str.str());
}

// This method will block until the connection is complete
void stat_socket::open_stat(const std::string& uri) {
  // Create a new connection to the given URI
  websocketpp::lib::error_code ec;
  client::connection_ptr con = m_client.get_connection(uri, ec);

  if (ec) {
    m_client.get_alog().write(websocketpp::log::alevel::app,
                              "Get Connection Error: " + ec.message());
    return;
  }

  // Grab a handle for this connection so we can talk to it in a thread
  // safe manor after the event loop starts.
  m_hdl = con->get_handle();

  // Queue the connection. No DNS queries or network connections will be
  // made until the io_service event loop is run.
  m_client.connect(con);

  // Create a thread to run the ASIO io_service event loop
  // websocketpp::lib::thread asio_thread(&client::run, &m_client);

  // Create a thread to run the telemetry loop
  // websocketpp::lib::thread telemetry_thread(&stat_socket::telemetry_loop,this);

  // asio_thread.join();
  // telemetry_thread.join();
}

void stat_socket::poll_one() {
  m_client.poll_one();
}

bool stat_socket::is_open() {
  scoped_lock guard(m_lock);

  return m_open;
}

// The open handler will signal that we are ready to start sending telemetry
void stat_socket::on_open(websocketpp::connection_hdl) {
  m_client.get_alog().write(websocketpp::log::alevel::app,
                            "Connection opened, starting telemetry!");

  scoped_lock guard(m_lock);
  m_open = true;
}

// The close handler will signal that we should stop sending telemetry
void stat_socket::on_close(websocketpp::connection_hdl) {
  m_client.get_alog().write(websocketpp::log::alevel::app,
                            "Connection closed, stopping telemetry!");

  scoped_lock guard(m_lock);
  m_done = true;
}

// The fail handler will signal that we should stop sending telemetry
void stat_socket::on_fail(websocketpp::connection_hdl) {
  m_client.get_alog().write(websocketpp::log::alevel::app,
                            "Connection failed, stopping telemetry!");

  scoped_lock guard(m_lock);
  m_done = true;
}

void stat_socket::send_stat(std::string val) {
  websocketpp::lib::error_code ec;
  m_client.get_alog().write(websocketpp::log::alevel::app, val);
  m_client.send(m_hdl, val, websocketpp::frame::opcode::text, ec);

  // The most likely error that we will get is that the connection is
  // not in the right state. Usually this means we tried to send a
  // message to a connection that was closed or in the process of
  // closing. While many errors here can be easily recovered from,
  // in this simple example, we'll stop the telemetry loop.
  if (ec) {
    m_client.get_alog().write(websocketpp::log::alevel::app,
                              "Send Error: " + ec.message());
  }
}

void stat_socket::telemetry_loop() {
  uint64_t count = 0;

  std::stringstream val;
  websocketpp::lib::error_code ec;

  while (1) {
    bool wait = false;

    {
      scoped_lock guard(m_lock);

      // If the connection has been closed, stop generating telemetry
      if (m_done) break;

      // If the connection hasn't been opened yet wait a bit and retry
      if (!m_open) {
        wait = true;
      }
    }

    if (wait) {
      sleep(1);
      continue;
    }

    val.str("");
    val << "count is " << count++;

    m_client.get_alog().write(websocketpp::log::alevel::app, val.str());
    m_client.send(m_hdl, val.str(), websocketpp::frame::opcode::text, ec);

    // The most likely error that we will get is that the connection is
    // not in the right state. Usually this means we tried to send a
    // message to a connection that was closed or in the process of
    // closing. While many errors here can be easily recovered from,
    // in this simple example, we'll stop the telemetry loop.
    if (ec) {
      m_client.get_alog().write(websocketpp::log::alevel::app,
                                "Send Error: " + ec.message());
      break;
    }

    sleep(1);
  }
}
