#include "StreamerClient.h"

using grpc::Channel;
using grpc::ChannelArguments;
using grpc::ClientContext;
using grpc::Status;
using streamer::TrunkRecorderStreamer;

StreamerClient::StreamerClient(){}

void StreamerClient::Init(std::shared_ptr<::grpc::Channel> channel, bool enable_audio_streams) {
    stub_ = streamer::TrunkRecorderStreamer::NewStub(channel);
    m_enable_audio_streams = enable_audio_streams;
    signal_writer_ = stub_->SendSignal(&signal_context_, &signal_response_);
    if(m_enable_audio_streams) {
        audio_writer_ = stub_->SendStream(&audio_context_, &audio_response_);
    }
}

void StreamerClient::Done() {
    signal_writer_->WritesDone();
    if(m_enable_audio_streams) {
        audio_writer_->WritesDone();
    }
}

void StreamerClient::SendAudio(const ::streamer::AudioSample& request) {
    audio_writer_->Write(request);
}

void StreamerClient::SendSignal(const ::streamer::SignalInfo& request) {
    signal_writer_->Write(request);
}

void StreamerClient::CallStarted(const ::streamer::CallInfo& request) {
    ::google::protobuf::Empty reply;
    ClientContext context;
    stub_->CallStarted(&context, request, &reply);
}

void StreamerClient::CallEnded(const ::streamer::CallInfo& request) {
    ::google::protobuf::Empty reply;
    ClientContext context;
    stub_->CallEnded(&context, request, &reply);
}

void StreamerClient::SetupRecorder(const ::streamer::RecorderInfo& request) {
    ::google::protobuf::Empty reply;
    ClientContext context;
    stub_->SetupRecorder(&context, request, &reply);
}

void StreamerClient::SetupSystem(const ::streamer::SystemInfo& request) {
    ::google::protobuf::Empty reply;
    ClientContext context;
    stub_->SetupSystem(&context, request, &reply);
}

void StreamerClient::SetupSource(const ::streamer::SourceInfo& request) {
    ::google::protobuf::Empty reply;
    ClientContext context;
    stub_->SetupSource(&context, request, &reply);
}

void StreamerClient::SetupConfig(const ::streamer::ConfigInfo& request) {
    ::google::protobuf::Empty reply;
    ClientContext context;
    stub_->SetupConfig(&context, request, &reply);
}
