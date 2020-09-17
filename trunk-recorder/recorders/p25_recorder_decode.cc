
#include "p25_recorder_decode.h"

p25_recorder_decode_sptr make_p25_recorder_decode(  int silence_frames) {
  p25_recorder_decode *decoder = new p25_recorder_decode();
  decoder->initialize(  silence_frames);
  return gnuradio::get_initial_sptr(decoder);
}

p25_recorder_decode::p25_recorder_decode()
    : gr::hier_block2("p25_recorder_decode",
                      gr::io_signature::make(1, 1, sizeof(float)),
                      gr::io_signature::make(0, 0, sizeof(float))) {
}

p25_recorder_decode::~p25_recorder_decode(){

}

void p25_recorder_decode::stop() {
        wav_sink->close();
}

void p25_recorder_decode::start(Call *call) {
    levels->set_k(call->get_system()->get_digital_levels());
    wav_sink->open(call->get_filename());
    wav_sink->set_call(call);
}
void p25_recorder_decode::set_xor_mask(const char *mask) {
    op25_frame_assembler->set_xormask(mask);
}
void p25_recorder_decode::set_tdma_slot(int slot) {


  tdma_slot = slot;
  op25_frame_assembler->set_slotid(tdma_slot);
}
double p25_recorder_decode::get_current_length() {
  return wav_sink->length_in_seconds();
}

void p25_recorder_decode::switch_tdma(bool phase2_tdma) {
    op25_frame_assembler->set_phase2_tdma(phase2_tdma);
}

void p25_recorder_decode::initialize(  int silence_frames) {
  //OP25 Slicer
  const float l[] = {-2.0, 0.0, 2.0, 4.0};
  std::vector<float> slices(l, l + sizeof(l) / sizeof(l[0]));
  slicer = gr::op25_repeater::fsk4_slicer_fb::make(slices);
  wav_sink = gr::blocks::nonstop_wavfile_sink_impl::make(1, 8000, 16, true);
  //OP25 Frame Assembler
  traffic_queue = gr::msg_queue::make(2);
  rx_queue = gr::msg_queue::make(100);

  int udp_port = 0;
  int verbosity = 0; // 10 = lots of debug messages
  const char *wireshark_host = "127.0.0.1";
  bool do_imbe = 1;
  bool do_output = 1;
  bool do_msgq = 0;
  bool do_audio_output = 1;
  bool do_tdma = 1;
  bool do_crypt = 0;

  op25_frame_assembler = gr::op25_repeater::p25_frame_assembler::make(0, silence_frames, wireshark_host, udp_port, verbosity, do_imbe, do_output, do_msgq, rx_queue, do_audio_output, do_tdma, do_crypt);
  converter = gr::blocks::short_to_float::make(1, 32768.0);
  levels = gr::blocks::multiply_const_ff::make(1);

    connect( self(),0, slicer,0);
  connect(slicer, 0, op25_frame_assembler, 0);
  connect(op25_frame_assembler, 0, converter, 0);
  connect(converter, 0, levels, 0);
  connect(levels, 0, wav_sink, 0);
}