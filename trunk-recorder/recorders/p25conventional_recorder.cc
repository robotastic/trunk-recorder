#include "p25_recorder.h"
#include "p25conventional_recorder.h"
#include "../../lib/gr_blocks/nonstop_wavfile_delayopen_sink_impl.h"
#include "../formatter.h"

p25conventional_recorder_sptr make_p25conventional_recorder(Source * src, bool delayopen)
{
  p25conventional_recorder * recorder = new p25conventional_recorder(delayopen);
  if (delayopen)
  {
    boost::shared_ptr<gr::blocks::nonstop_wavfile_delayopen_sink_impl> w = gr::blocks::nonstop_wavfile_delayopen_sink_impl::make(1, 8000, 16, true);
    w->set_recorder(recorder);
    recorder->initialize(src, w);
  }
  else
  {
    boost::shared_ptr<gr::blocks::nonstop_wavfile_sink_impl> w = gr::blocks::nonstop_wavfile_sink_impl::make(1, 8000, 16, true);
    recorder->initialize(src, w);
  }
  return gnuradio::get_initial_sptr<p25conventional_recorder>(recorder);
}

p25conventional_recorder::p25conventional_recorder(bool delayopen) : p25_recorder("P25C")
{
  d_delayopen = delayopen;
}

p25conventional_recorder::~p25conventional_recorder()
{

}

void p25conventional_recorder::start(Call *call) {
  if (state == inactive) {
    timestamp = time(NULL);
    starttime = time(NULL);

    talkgroup = call->get_talkgroup();
    short_name = call->get_short_name();
    chan_freq      = call->get_freq();
    this->call = call;

    if (d_delayopen) {
      boost::static_pointer_cast<gr::blocks::nonstop_wavfile_delayopen_sink_impl>(this->wav_sink)->reset();
    }

    //((gr::blocks::nonstop_wavfile_delayopen_sink_impl *)this->wav_sink)->reset();

    set_tdma(call->get_phase2_tdma());

    if (call->get_phase2_tdma()) {

      set_tdma_slot(call->get_tdma_slot());

      if (call->get_xor_mask()) {
        op25_frame_assembler->set_xormask(call->get_xor_mask());
      } else {
          BOOST_LOG_TRIVIAL(info) << "Error - can't set XOR Mask for TDMA";
      }
    } else {
      set_tdma_slot(0);
    }

    if (!qpsk_mod) {
      reset();
    }
    if (d_delayopen) {
      BOOST_LOG_TRIVIAL(info) << "\t- Listening P25 Recorder Num [" << rec_num << "]\tTG: " << this->call->get_talkgroup_display() << "\tFreq: " << FormatFreq(chan_freq) << " \tTDMA: " << call->get_phase2_tdma() << "\tSlot: " << call->get_tdma_slot();
    } else {
      recording_started();
    }


    int offset_amount = (chan_freq - center_freq);
    prefilter->set_center_freq(offset_amount);
    if (d_delayopen == false) {
      wav_sink->open(call->get_filename());
    }
    state = active;
    valve->set_enabled(true);
  } else {
    BOOST_LOG_TRIVIAL(error) << "p25conventional_recorder.cc: Trying to Start an already Active Logger!!!";
  }
}

void p25conventional_recorder::recording_started()
{
    BOOST_LOG_TRIVIAL(info) << "\t- Started P25 Recorder Num [" << rec_num << "]\tTG: " << this->call->get_talkgroup_display() << "\tFreq: " << FormatFreq(chan_freq) << " \tTDMA: " << call->get_phase2_tdma() << "\tSlot: " << call->get_tdma_slot();
}

char * p25conventional_recorder::get_filename() {
  this->call->create_filename();
  return this->call->get_filename();
}
