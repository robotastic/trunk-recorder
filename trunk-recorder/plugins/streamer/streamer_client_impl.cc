#include "../plugin-common.h"
#include "streamer_proto_helper.h"
#include <grpcpp/grpcpp.h>
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

int init(plugin_t * const plugin, Config* config) {
    //
}
int parse_config(plugin_t * const plugin, boost::property_tree::ptree::value_type &cfg) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->state;

    plug_data->enable_audio_streaming = cfg->get<bool>("enable_audio_streaming", false);
    plug_data->server_addr = cfg->get<std::string>("server_address", "");
}

int start(plugin_t * const plugin) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->state;

    plug_data->client = StreamerClient(grpc::CreateChannel(plug_data->server_addr, grpc::InsecureChannelCredentials()), plub_data->enable_audio_streaming);
}
int stop(plugin_t * const plugin) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->state;

    plug_data->client = NULL;
}
int poll_one(plugin_t * const plugin){}
int signal(plugin_t * const plugin, long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->state;

    streamer::SignalInfo* sig = ToSignalInfo(unitId, signaling_type, sig_type, call, system, recorder);
    if(sig != NULL && plug_data->client != NULL) {
        plug_data->client->SendSignal(sig);
    }
}
int audio_stream(plugin_t * const plugin, Recorder *recorder, float *samples, int sampleCount) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->state;

    if(plug_data->client == NULL || !plug_data->enable_audio_streaming) return;

    streamer::AudioSample* s = ToAudioSample(recorder, samples, sampleCount);
    if(s != NULL)
        plug_data->client->SendAudio(s);
}
int call_start(plugin_t * const plugin, Call *call) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->state;

    streamer::CallInfo* c = ToCallInfo(call);
    if(plug_data->client != NULL && c != NULL)
        plug_data->client->CallStarted(c);
}
int call_end(plugin_t * const plugin, Call *call) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->state;

    streamer::CallInfo* c = ToCallInfo(call);
    if(plug_data->client != NULL && c != NULL)
        plug_data->client->CallEnded(c);
}
int calls_active(plugin_t * const plugin, std::vector<Call *> calls){}
int setup_recorder(plugin_t * const plugin, Recorder *recorder) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->state;

    streamer::RecorderInfo* c = ToRecorderInfo(recorder);
    if(plug_data->client != NULL && c != NULL)
        plug_data->client->SetupRecorder(c);
}
int setup_system(plugin_t * const plugin, System * system) {
    streamer_client_plugin_t* plug_data = (streamer_client_plugin_t*)plugin->state;

    streamer::SystemInfo* c = ToSystemInfo(system);
    if(plug_data->client != NULL && c != NULL)
        plug_data->client->SetupSystem(c);
}
int setup_systems(plugin_t * const plugin, std::vector<System *> systems){}
int setup_sources(plugin_t * const plugin, std::vector<Source *> sources){}
int setup_config(plugin_t * const plugin, std::vector<Source *> sources, std::vector<System *> systems){}
int system_rates(plugin_t * const plugin, std::vector<System *> systems, float timeDiff){}

MODULE_EXPORT plugin_t *streamer_client_plugin_new() {
    streamer_client_plugin_t* plug_info = (streamer_client_plugin_t*)malloc(sizeof(streamer_client_plugin_t));

    plugin_t *plug_data = (plugin_t *)malloc(sizeof(plugin_t));
    plug_data->state = plug_info;
    plug_data->init = init;
    plug_data->parse_config = parse_config;
    plug_data->start = start;
    plug_data->stop = stop;
    plug_data->poll_one = poll_one;
    plug_data->signal = signal;
    plug_data->audio_stream = audio_stream;
    plug_data->call_start = call_start;
    plug_data->call_end = call_end;
    plug_data->calls_active = calls_active;
    plug_data->setup_recorder = setup_recorder;
    plug_data->setup_system = setup_system;
    plug_data->setup_systems = setup_systems;
    plug_data->setup_sources = setup_sources;
    plug_data->setup_config = setup_config;
    plug_data->system_rates = system_rates;

    return plug_data;
}