#ifndef STREAMERCLIENT_H
#define STREAMERCLIENT_H

#include <iostream>
#include <memory>
#include <string>
#include <stdbool.h>

#include <grpcpp/grpcpp.h>
#include "streamer.grpc.pb.h"
#include "streamer.pb.h"

class StreamerClient {
    public:
        StreamerClient();

        void Init(std::shared_ptr<::grpc::Channel> channel, bool enable_audio_streams);
        void Done();

        void SendAudio(const ::streamer::AudioSample& request);
        void SendSignal(const ::streamer::SignalInfo& request);
        void CallStarted(const ::streamer::CallInfo& request);
        void CallEnded(const ::streamer::CallInfo& request);
        void SetupRecorder(const ::streamer::RecorderInfo& request);
        void SetupSystem(const ::streamer::SystemInfo& request);
        void SetupSource(const ::streamer::SourceInfo& request);
        void SetupConfig(const ::streamer::ConfigInfo& request);

    private:
        bool m_enable_audio_streams;
        std::unique_ptr< ::streamer::TrunkRecorderStreamer::Stub> stub_;
        std::unique_ptr< ::grpc::ClientWriter< ::streamer::SignalInfo>> signal_writer_;
        std::unique_ptr< ::grpc::ClientWriter< ::streamer::AudioSample>> audio_writer_;
        ::grpc::ClientContext signal_context_;
        ::grpc::ClientContext audio_context_;
        ::google::protobuf::Empty signal_response_;
        ::google::protobuf::Empty audio_response_;
};

#endif // STREAMERCLIENT_H