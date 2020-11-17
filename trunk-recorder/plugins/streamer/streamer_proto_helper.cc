#include "streamer_proto_helper.h"
#include "../../state.h"
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <google/protobuf/util/time_util.h>

streamer::AudioSample* ToAudioSample(Recorder *recorder, float *samples, int sampleCount) {
    streamer::AudioSample* a = streamer::AudioSample::New();
    a->set_sample_time(google::protobuf::util::TimeUtil::GetCurrentTime());
    a->set_sample(samples, sampleCount);
    if(recorder != NULL) {
        a->set_recorder_num(recorder->get_num());
    }

    return a;
}

streamer::CallInfo* ToCallInfo(Call *call) {
    if(call == NULL) return NULL;

    streamer::CallInfo* ci = streamer::CallInfo::New();
    ci->set_call_active(call->get_state() == inactive);
    ci->set_recorder_num(call->get_recorder()->get_num());
    ci->set_system_num(call->get_sys_num());
    ci->set_curr_freq(call->get_freq());
    //
    return ci;
}

streamer::SystemInfo* ToSystemInfo(System *system) {
    if(system == NULL) return NULL;
    streamer::SystemInfo* si = streamer::SystemInfo::New();
    si->set_system_num(system->get_sys_num());
    si->set_system_type(system->get_system_type());
    si->set_system_name(system->get_short_name());
    si->set_audio_archive(system->get_audio_archive());
    si->set_upload_script(system->get_upload_script());
    si->set_record_unknown(system->get_record_unknown());
    si->set_call_log(system->get_call_log());
    si->set_talkgroups_file(system->get_talkgroups_file());
    si->set_analog_levels(system->get_analog_levels());
    si->set_digital_levels(system->get_digital_levels());
    si->set_qpsk(system->get_qpsk_mod());
    si->set_squelch_db(system->get_squelch_db());

    std::vector<double> channels;

    if ((system->get_system_type() == "conventional") || (system->get_system_type() == "conventionalP25")) {
      channels = system->get_channels();
    } else {
      channels = system->get_control_channels();
    }

    for(std::vector<double>::iterator it = channels.begin(); it != channels.end(); it++) {
        double f = *it;
        si->add_channels(f);
    }

    si->set_bandplan(system->get_bandplan());
    si->set_bandfreq(system->get_bandfreq();)
    si->set_bandplan_base(system->get_bandplan_base());
    si->set_bandplan_high(system->get_bandplan_high());
    si->set_bandplan_spacing(system->get_bandplan_spacing());
    si->set_bandplan_offset(system->get_bandplan_offset());
    /*
    string talkgroups_file = 8;
    double analog_levels = 9;
    double digital_levels = 10;
    bool qpsk = 11;
    double squelch_db = 12;
    repeated double channels = 13;
    string bandplan = 14;
    int32 bandfreq = 15;
    double bandplan_base = 16;
    double bandplan_high = 17;
    double bandplan_spacing = 18;
    int32 bandplan_offset = 19;*/
    //
    return si;
}

streamer::SourceInfo* ToSourceInfo(Source *source) { 
    if(source == NULL) return NULL;
    streamer::SourceInfo* si = streamer::SourceInfo::New();
    si->set_source_num(source->get_num());
    si->set_min_hz(source->get_min_hz());
    si->set_max_hz(source->get_max_hz());
    si->set_center_hz(source->get_center());
    si->set_rate(source->get_rate());
    si->set_driver(source->get_driver());
    si->set_device(source->get_device());
    si->set_antenna(source->get_antenna());
    si->set_error(source->get_error());
    si->set_mix_gain(source->get_mix_gain());
    si->set_lna_gain(source->get_lna_gain());
    si->set_vga1_gain(source->get_vga1_gain());
    si->set_vga2_gain(source->get_vga2_gain());
    si->set_b_gain(source->get_bb_gain());
    si->set_gain(source->get_gain());
    si->set_if_gain(source->get_if_gain());
    si->set_analog_recorders(source->analog_recorder_count());
    si->set_digital_recorders(source->digital_recorder_count());
    si->set_debug_recorders(source->debug_recorder_count());
    si->set_sigmf_recorders(source->sigmf_recorder_count());
    //
    return si;
}

streamer::RecorderInfo* ToRecorderInfo(Recorder *recorder) {
    if(recorder == NULL) return NULL;

    streamer::RecorderInfo* ri = streamer::RecorderInfo::New();
    ri->set_recorder_num(recorder->get_num());
    ri->set_recorder_type(recorder->get_type();
    ri->set_source_num(recorder->get_source()->get_num());
    ri->set_id(boost::lexical_cast<std::string>(recorder->get_source()->get_num()) + "_" + boost::lexical_cast<std::string>(recorder->get_num()));
    ri->set_recorder_count(recorder->get_recording_count());
    ri->set_recorder_duration(recorder->get_recorder_duration());
    //
    return ri;
}

::streamer::SignalInfo_SignalType ToSignalType(gr::blocks::SignalType sig_type) {
    switch(sig_type) {
        case gr::blocks::SignalType::Emergency: return ::streamer::SignalInfo_SignalType::Emergency;
        case gr::blocks::SignalType::EmergencyAck: return ::streamer::SignalInfo_SignalType::EmergencyAck;
        case gr::blocks::SignalType::EmergencyPre: return ::streamer::SignalInfo_SignalType::EmergencyPre;
        case gr::blocks::SignalType::RadioCheck: return ::streamer::SignalInfo_SignalType::RadioCheck;
        case gr::blocks::SignalType::RadioCheckAck: return ::streamer::SignalInfo_SignalType::RadioCheckAck;
        case gr::blocks::SignalType::RadioStun: return ::streamer::SignalInfo_SignalType::RadioStun;
        case gr::blocks::SignalType::RadioStunAck: return ::streamer::SignalInfo_SignalType::RadioStunAck;
        case gr::blocks::SignalType::RadioRevive: return ::streamer::SignalInfo_SignalType::RadioRevive;
        case gr::blocks::SignalType::RadioReviveAck: return ::streamer::SignalInfo_SignalType::RadioReviveAck;
        case gr::blocks::SignalType::NormalPre: return ::streamer::SignalInfo_SignalType::NormalPre;
        default: return ::streamer::SignalInfo_SignalType::Normal;
    }
}

streamer::SignalInfo* ToSignalInfo(long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder) {
    streamer::SignalInfo* si = streamer::StreamerInfo::New();
    si->set_unit_id(unitId);
    si->set_signaling_type(signaling_type);
    si->set_signal_type(ToSignalType(sig_type));
    si->set_call_info(ToCallInfo(call));
    si->set_system_info(ToSystemInfo(system));
    si->set_recorder_info(ToRecorderInfo(recorder));
    return si;
}