#include "stat_socket.h"

/**
 * The telemetry client connects to a WebSocket server and sends a message every
 * second containing an integer count. This example can be used as the basis for
 * programs where a client connects and pushes data for logging, stress/load
 * testing, etc.
 */
void stat_socket::send_sys_rates(std::vector<System *>systems, float timeDiff) {

  if (m_open == false)
    return;
  boost::property_tree::ptree nodes;

  for (std::vector<System *>::iterator it = systems.begin(); it != systems.end(); it++) {
    System *system         = *it;
    nodes.push_back(std::make_pair("", system->get_stats_current(timeDiff)));
  }
  send_object(nodes, "rates", "rates");
}

stat_socket::stat_socket() : m_open(false), m_done(false), m_config_sent(false) {
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

void stat_socket::send_config(std::vector<Source *>sources, std::vector<System *>systems) {

  if (m_open == false)
    return;

  if (config_sent())
    return;

  boost::property_tree::ptree root;
  boost::property_tree::ptree systems_node;
  boost::property_tree::ptree sources_node;

  for (std::vector<Source *>::iterator it = sources.begin(); it != sources.end(); it++) {
    Source *source = *it;
    boost::property_tree::ptree source_node;
    source_node.put("source_num",           source->get_num());
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
    source_node.put("vga1_gain",         source->get_vga1_gain());
    source_node.put("vga2_gain",         source->get_vga2_gain());
    source_node.put("bb_gain",           source->get_bb_gain());
    source_node.put("gain",              source->get_gain());
    source_node.put("if_gain",           source->get_if_gain());
    source_node.put("squelch_db",        source->get_squelch_db());
    source_node.put("antenna",           source->get_antenna());
    source_node.put("analog_recorders",  source->analog_recorder_count());
    source_node.put("digital_recorders", source->digital_recorder_count());
    source_node.put("debug_recorders",   source->debug_recorder_count());
    source_node.put("sigmf_recorders",   source->sigmf_recorder_count());
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

    //std::cout << "starts: " << std::endl;

    for (std::vector<double>::iterator chan_it = channels.begin(); chan_it != channels.end(); chan_it++) {
      double channel = *chan_it;
      boost::property_tree::ptree channel_node;
      //std::cout << "Hello: " << channel << std::endl;
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
  root.put("captureDir",    m_config->capture_dir);
  root.put("uploadServer",  m_config->upload_server);

  // root.put("defaultMode", default_mode);
  root.put("callTimeout",   m_config->call_timeout);
  root.put("logFile",       m_config->log_file);
  root.put("instanceId",    m_config->instance_id);
  root.put("instanceKey",   m_config->instance_key);
  root.put("logFile",       m_config->log_file);
  root.put("type",         "config");
  std::stringstream stats_str;
  boost::property_tree::write_json(stats_str, root);

  // std::cout << stats_str;
  send_stat(stats_str.str());
  m_config_sent = true;
}


void stat_socket::send_systems(std::vector<System *> systems) {
  if (m_open == false)
    return;
  boost::property_tree::ptree node;

  for (std::vector<System *>::iterator it = systems.begin(); it != systems.end(); it++) {
    System *system         = *it;
    node.push_back(std::make_pair("", system->get_stats()));
  }
  send_object(node, "systems", "systems");
}

void stat_socket::send_system(System * system) {
  if (m_open == false)
    return;

  send_object(system->get_stats(), "system", "system");
}


void stat_socket::send_calls_active(std::vector<Call *>calls) {
  if (m_open == false)
    return;
  boost::property_tree::ptree node;

  for (std::vector<Call *>::iterator it = calls.begin(); it != calls.end(); it++) {
    Call *call         = *it;
    if (call->get_state() == recording) {
      node.push_back(std::make_pair("", call->get_stats()));
    }
  }

  send_object(node, "calls", "calls_active");
}

void stat_socket::send_recorders(std::vector<Recorder *>recorders) {

  if (m_open == false)
    return;
  boost::property_tree::ptree node;

  for (std::vector<Recorder *>::iterator it = recorders.begin(); it != recorders.end(); it++) {
    Recorder *recorder         = *it;
    node.push_back(std::make_pair("", recorder->get_stats()));
  }

  send_object(node, "recorders", "recorders");
}

void stat_socket::send_call_start(Call * call)
{
    if (m_open == false)
      return;

  send_object(call->get_stats(), "call", "call_start");
}

void stat_socket::send_call_end(Call * call)
{
    if (m_open == false)
      return;

  send_object(call->get_stats(), "call", "call_end");
}

void stat_socket::send_recorder(Recorder * recorder)
{
    if (m_open == false)
      return;

  send_object(recorder->get_stats(), "recorder", "recorder");
}


void stat_socket::send_object(boost::property_tree::ptree data, std::string name, std::string type)
{
  if (m_open == false)
    return;
  boost::property_tree::ptree root;

  root.add_child(name, data);
  root.put("type", type);
  root.put("instanceId",      m_config->instance_id);
  root.put("instanceKey",     m_config->instance_key);
  std::stringstream stats_str;
  boost::property_tree::write_json(stats_str, root);

  // std::cout << stats_str;
  send_stat(stats_str.str());
}

void stat_socket::initialize(Config * config, void (*callback)(void))
{
  m_config = config;
  m_callback = callback;
}

void stat_socket::reopen_stat() {
  m_client.reset();
  m_reconnect = false;
  open_stat();
}
// This method will block until the connection is complete
void stat_socket::open_stat() {
  // Create a new connection to the given URI

  if (this->m_config->status_server == "") {
    return;
  }

  websocketpp::lib::error_code ec;
  client::connection_ptr con = m_client.get_connection(this->m_config->status_server, ec);

  if (ec) {
    m_client.get_alog().write(websocketpp::log::alevel::app,  "open_stat: Get WebSocket Connection Error: " + ec.message());
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
  if (m_reconnect && (reconnect_time - time(NULL) < 0)) {
    reopen_stat();
  }
  m_client.poll_one();
}

bool stat_socket::is_open() {
  scoped_lock guard(m_lock);

  return m_open;
}

bool stat_socket::config_sent() {
  scoped_lock guard(m_lock);

  return m_config_sent;
}
// The open handler will signal that we are ready to start sending telemetry
void stat_socket::on_open(websocketpp::connection_hdl) {
  m_client.get_alog().write(websocketpp::log::alevel::app,
                            "on_open: WebSocket Connection opened, starting telemetry!");

  {
    // de scope the lock before calling the callback
    scoped_lock guard(m_lock);
    m_open = true;
    retry_attempt = 0;
  }
  m_callback();
}

// The close handler will signal that we should stop sending telemetry
void stat_socket::on_close(websocketpp::connection_hdl) {
  std::stringstream stream_num;
  std::string str_num;
  m_client.get_alog().write(websocketpp::log::alevel::app,
                            "on_close: WebSocket Connection closed, stopping telemetry!");

  scoped_lock guard(m_lock);
  m_open = false;
  m_done = true;
  m_config_sent = false;
  m_reconnect = true;
  retry_attempt++;
  long reconnect_delay = (6 * retry_attempt + (rand() % 30));
  stream_num << reconnect_delay;
  stream_num >> str_num;
  reconnect_time = time( NULL) + reconnect_delay;
  m_client.get_alog().write(websocketpp::log::alevel::app,  "on_close: Will try to reconnect in:  " + str_num);
}

// The fail handler will signal that we should stop sending telemetry
void stat_socket::on_fail(websocketpp::connection_hdl) {
  std::stringstream stream_num;
  std::string str_num;
  m_client.get_alog().write(websocketpp::log::alevel::app,  "on_fail: WebSocket Connection failed, stopping telemetry!");

  scoped_lock guard(m_lock);
  m_open = false;
  m_done = true;
  m_config_sent = false;
  if (!m_reconnect) {
    m_reconnect = true;
    retry_attempt++;
    long reconnect_delay = (6 * retry_attempt + (rand() % 30));
    stream_num << reconnect_delay;
    stream_num >> str_num;
    reconnect_time = time( NULL) + reconnect_delay;
    m_client.get_alog().write(websocketpp::log::alevel::app,  "on_fail: Will try to reconnect in:  " + str_num);
  }
}

void stat_socket::send_stat(std::string val) {
  websocketpp::lib::error_code ec;
  if (m_open) {
    m_client.send(m_hdl, val, websocketpp::frame::opcode::text, ec);

    // The most likely error that we will get is that the connection is
    // not in the right state. Usually this means we tried to send a
    // message to a connection that was closed or in the process of
    // closing. While many errors here can be easily recovered from,
    // in this simple example, we'll stop the telemetry loop.
    if (ec) {
      m_client.get_alog().write(websocketpp::log::alevel::app,
                                "Error Sending : " + ec.message());
    }
  }
}
