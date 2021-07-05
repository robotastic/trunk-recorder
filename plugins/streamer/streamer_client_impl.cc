#include "../trunk-recorder/plugin_manager/plugin_common.h"
#include "streamer_client_impl.h"
#include "streamer_proto_helper.h"
#include <grpcpp/grpcpp.h>
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

int streamer_init(plugin_t * const plugin, Config* config) {
    return 0;
}
int streamer_parse_config(plugin_t * const plugin, boost::property_tree::ptree::value_type &cfg) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->plugin_data;

    //plug_data->enable_audio_streaming = cfg.get<bool>("enable_audio_streaming", false);
    //plug_data->server_addr = cfg.get<std::string>("server_address", "");
    return 0;
}

int streamer_start(plugin_t * const plugin) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->plugin_data;

    plug_data->client.Init(grpc::CreateChannel(plug_data->server_addr, grpc::InsecureChannelCredentials()), plug_data->enable_audio_streaming);

    return 0;
}
int streamer_stop(plugin_t * const plugin) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->plugin_data;

    plug_data->client.Done();

    return 0;
}
int streamer_poll_one(plugin_t * const plugin){
    return 0;
}
int streamer_signal(plugin_t * const plugin, long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->plugin_data;

    streamer::SignalInfo* sig = ToSignalInfo(unitId, signaling_type, sig_type, call, system, recorder);
    plug_data->client.SendSignal(*sig);
    
    return 0;
}

int streamer_audio_stream(plugin_t * const plugin, Recorder *recorder, float *samples, int sampleCount) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->plugin_data;

    if(!plug_data->enable_audio_streaming) return -1;

    streamer::AudioSample* s = ToAudioSample(recorder, samples, sampleCount);
    plug_data->client.SendAudio(*s);

    return 0;
}
int streamer_call_start(plugin_t * const plugin, Call *call) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->plugin_data;

    streamer::CallInfo* c = ToCallInfo(call);
    plug_data->client.CallStarted(*c);

    return 0;
}
int streamer_call_end(plugin_t * const plugin, Call *call) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->plugin_data;

    streamer::CallInfo* c = ToCallInfo(call);
    plug_data->client.CallEnded(*c);

    return 0;
}
int streamer_calls_active(plugin_t * const plugin, std::vector<Call *> calls){
    return 0;
}
int streamer_setup_recorder(plugin_t * const plugin, Recorder *recorder) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->plugin_data;

    streamer::RecorderInfo* c = ToRecorderInfo(recorder);
    plug_data->client.SetupRecorder(*c);

    return 0;
}
int streamer_setup_system(plugin_t * const plugin, System * system) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->plugin_data;

    streamer::SystemInfo* c = ToSystemInfo(system);
    plug_data->client.SetupSystem(*c);

    return 0;
}
int streamer_setup_systems(plugin_t * const plugin, std::vector<System *> systems){
    return 0;
}
int streamer_setup_sources(plugin_t * const plugin, std::vector<Source *> sources){
    return 0;
}
int streamer_setup_config(plugin_t * const plugin, std::vector<Source *> sources, std::vector<System *> systems){
    return 0;
}
int streamer_system_rates(plugin_t * const plugin, std::vector<System *> systems, float timeDiff){
    return 0;
}

MODULE_EXPORT plugin_t *streamer_client_plugin_new() {
    streamer_client_plugin_t* plug_info = (streamer_client_plugin_t*)malloc(sizeof(streamer_client_plugin_t));

    plugin_t *plug_data = (plugin_t *)malloc(sizeof(plugin_t));
    plug_data->plugin_data = plug_info;
    plug_data->init = streamer_init;
    plug_data->parse_config = streamer_parse_config;
    plug_data->start = streamer_start;
    plug_data->stop = streamer_stop;
    plug_data->poll_one = streamer_poll_one;
    plug_data->signal = streamer_signal;
    plug_data->audio_stream = streamer_audio_stream;
    plug_data->call_start = streamer_call_start;
    plug_data->call_end = streamer_call_end;
    plug_data->calls_active = streamer_calls_active;
    plug_data->setup_recorder = streamer_setup_recorder;
    plug_data->setup_system = streamer_setup_system;
    plug_data->setup_systems = streamer_setup_systems;
    plug_data->setup_sources = streamer_setup_sources;
    plug_data->setup_config = streamer_setup_config;
    plug_data->system_rates = streamer_system_rates;

    return plug_data;
}