Plugin System
=============

The plugin system is designed to allow third parties to integrate their own features without changing the core code of trunk-recorder, and allow for features to be updated independently of the trunk-recorder code base. The current methods supported by the plugin system is loosely based on the Status uploader.

*Unless otherwise noted, all fuctions need to return an int result (0 for successful, -1 for failure)*

Plugins need to implement the following methods:
* `<name>_plugin_new`
  * Returns a new pointer to a plugin_t instance, along with the used callback method references.

* `init(plugin_t * const plugin)`
  * The plugin_t instance needs to have a reference to an init(plugin_t) call.
  * The init(plugin_t) method gets called after the parse_config method.

Plugins can *optionally* implement the following methods, based on usage, or set to NULL:
* `parse_config(plugin_t * const plugin, boost::property_tree::ptree::value_type &cfg)`
  * Called before init(plugin_t), and passed the Configuration information in the settings file for that plugin.
  
* `start(plugin_t * const plugin)`
  * Called after trunk-recorder has been setup and all configuraiton is loaded.

* `stop(plugin_t * const plugin)`
  * Called after trunk-recorder has started shutting down, but the top-block is still running.

* `poll_one(plugin_t * const plugin)`
  * Called during each pass thru the main loop of trunk-recorder.

* `call_start(plugin_t * const plugin, Call *call)`
  * Called when a new call is starting.

* `call_end(plugin_t * const plugin, Call_Data_t call_info)`
  * Called when a call has ended.

* `trunk_message(std::vector<TrunkMessage> messages, System *system)`
  * Called when a new message is received from the control channel of a Trunk system

* `setup_recorder(plugin_t * const plugin, Recorder *recorder)`
  * Called when a new recorder has been created.

* `setup_system(plugin_t * const plugin, System * system)`
  * Called when a new system has been created.

* `setup_systems(plugin_t * const plugin, std::vector<System *> systems)`
  * Called during startup when the initial systems have been created.

* `setup_sources(plugin_t * const plugin, std::vector<Source *> sources)`
  * Called during startup when the initial sources have been created.
    
* `signal(plugin_t * const plugin, long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder)`
  * Called when a decoded signal (i.e. MDC-1200) has been detected.

* `audio_stream(plugin_t * const plugin, Call *call, Recorder *recorder, float *samples, int sampleCount)`
  * Called when a set of audio samples that would be written out to the wav file writer is available.
  * Useful to implement live audio streaming.

*  `unit_registration(System *sys, long source_id)`
  * Called when a Subscriber Unit (radio) registers with a Trunk System

*  `unit_deregistration(System *sys, long source_id)`
  * Called when a Subscriber Unit (radio) deregisters from a Trunk System

*  `unit_acknowledge_response(System *sys, long source_id)`
  * Called for the Unit Ack Trunk message

*  `unit_group_affiliation(System *sys, long source_id, long talkgroup_num)`
  * Called when a new Subscriber Unit (radio) selects a Talkgroup on a Trunk System

*  `unit_data_grant(System *sys, long source_id)`
  * Called for the Unit Data Grant Trunk Message

*  `unit_answer_request(System *sys, long source_id, long talkgroup)`
  * Called for the Unit Answer Request Trunk Message

*  `unit_location(System *sys, long source_id, long talkgroup_num)`
  * Called for the Unit Location Trunk Message
