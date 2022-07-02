
#include "p25_recorder_decode.h"
#include "../gr_blocks/plugin_wrapper_impl.h"
#include "../plugin_manager/plugin_manager.h"

p25_recorder_decode_sptr make_p25_recorder_decode(Recorder *recorder, int silence_frames) {
  p25_recorder_decode *decoder = new p25_recorder_decode(recorder);
  decoder->initialize(silence_frames);
  return gnuradio::get_initial_sptr(decoder);
}

p25_recorder_decode::p25_recorder_decode(Recorder *recorder)
    : gr::hier_block2("p25_recorder_decode",
                      gr::io_signature::make(1, 1, sizeof(float)),
                      gr::io_signature::make(0, 0, sizeof(float))) {
  d_recorder = recorder;
}

p25_recorder_decode::~p25_recorder_decode() {
}

void p25_recorder_decode::stop() {
  wav_sink->stop_recording();
}

void p25_recorder_decode::start(Call *call) {
  levels->set_k(call->get_system()->get_digital_levels());
  wav_sink->start_recording(call);
  d_call = call;
}

void p25_recorder_decode::set_xor_mask(const char *mask) {
  op25_frame_assembler->set_xormask(mask);
}
void p25_recorder_decode::set_record_more_transmissions(bool more) {
  wav_sink->set_record_more_transmissions(more);
}

void p25_recorder_decode::set_source(long src) {
  wav_sink->set_source(src);
}

std::vector<Transmission> p25_recorder_decode::get_transmission_list() {
  return wav_sink->get_transmission_list();
}

void p25_recorder_decode::set_tdma_slot(int slot) {

  tdma_slot = slot;
  op25_frame_assembler->set_slotid(tdma_slot);
}
double p25_recorder_decode::get_current_length() {
  return wav_sink->total_length_in_seconds();
}

State p25_recorder_decode::get_state() {
  return wav_sink->get_state();
}

double p25_recorder_decode::since_last_write() {
  time_t now = time(NULL);
  return now - wav_sink->get_stop_time();
}

void p25_recorder_decode::switch_tdma(bool phase2_tdma) {
  op25_frame_assembler->set_phase2_tdma(phase2_tdma);
}

void p25_recorder_decode::initialize(int silence_frames) {
  // OP25 Slicer
  const float l[] = {-2.0, 0.0, 2.0, 4.0};
  std::vector<float> slices(l, l + sizeof(l) / sizeof(l[0]));
  const int msgq_id = 0;
  const int debug = 0;
  slicer = gr::op25_repeater::fsk4_slicer_fb::make(msgq_id, debug, slices);
  wav_sink = gr::blocks::transmission_sink::make(1, 8000, 16);
  // recorder->initialize(src);

  bool use_streaming = d_recorder->get_enable_audio_streaming();

  // OP25 Frame Assembler
  traffic_queue = gr::msg_queue::make(2);
  rx_queue = gr::msg_queue::make(100);

  int udp_port = 0;
  int verbosity = 0; // 10 = lots of debug messages
  const char *udp_host = "127.0.0.1";
  bool do_imbe = 1;
  bool do_output = 1;
  bool do_msgq = 0;
  bool do_audio_output = 1;
  bool do_tdma = 0;
  bool do_nocrypt = 1;

  op25_frame_assembler = gr::op25_repeater::p25_frame_assembler::make(0, silence_frames, udp_host, udp_port, verbosity, do_imbe, do_output, do_msgq, rx_queue, do_audio_output, do_tdma, do_nocrypt);

  levels = gr::blocks::multiply_const_ss::make(1);

  if (use_streaming) {
    plugin_sink = gr::blocks::plugin_wrapper_impl::make(std::bind(&p25_recorder_decode::plugin_callback_handler, this, std::placeholders::_1, std::placeholders::_2));
  }

  connect(self(), 0, slicer, 0);
  connect(slicer, 0, op25_frame_assembler, 0);
  connect(op25_frame_assembler, 0, levels, 0);

  if (use_streaming) {
    connect(levels, 0, plugin_sink, 0);
  }

  connect(levels, 0, wav_sink, 0);
}

void p25_recorder_decode::plugin_callback_handler(int16_t *samples, int sampleCount) {
  plugman_audio_callback(d_call, d_recorder, samples, sampleCount);
}

double p25_recorder_decode::get_output_sample_rate() {
  return 8000;
}
