#include "p25conventional_recorder.h"
#include "../../lib/gr_blocks/nonstop_wavfile_delayopen_sink_impl.h"
#include "../formatter.h"
#include "p25_recorder.h"

p25conventional_recorder_sptr make_p25conventional_recorder(Source *src, bool delayopen) {
  p25conventional_recorder *recorder = new p25conventional_recorder(delayopen);

  return gnuradio::get_initial_sptr<p25conventional_recorder>(recorder);
}

p25conventional_recorder::p25conventional_recorder(bool delayopen) : p25_recorder("P25C") {
  d_delayopen = delayopen;
}

p25conventional_recorder::~p25conventional_recorder() {
}



void p25conventional_recorder::start(Call *call) {
  if (state == inactive) {
    System *system = call->get_system();
    timestamp = time(NULL);
    starttime = time(NULL);

    call->clear_src_list();
    talkgroup = call->get_talkgroup();
    short_name = call->get_short_name();
    chan_freq = call->get_freq();
    this->call = call;

    squelch_db = system->get_squelch_db();
    squelch->set_threshold(squelch_db);
    qpsk_mod = system->get_qpsk_mod();

    set_tdma(call->get_phase2_tdma());
    /*
    if (d_delayopen) {  
      if (qpsk_mod) {
        qpsk_p25_decode->reset_wav_sink();
      } else {
        fsk4_p25_decode->reset_wav_sink();
      }
    }
*/
    //((gr::blocks::nonstop_wavfile_delayopen_sink_impl *)this->wav_sink)->reset();

    set_tdma(call->get_phase2_tdma());

    if (call->get_phase2_tdma()) {

      set_tdma_slot(call->get_tdma_slot());

      if (call->get_xor_mask()) {
        //op25_frame_assembler->set_xormask(call->get_xor_mask());
      } else {
        BOOST_LOG_TRIVIAL(info) << "Error - can't set XOR Mask for TDMA";
      }
    } else {
      set_tdma_slot(0);
    }

 

    if (d_delayopen) {
      BOOST_LOG_TRIVIAL(info) << "\t- Listening P25 Recorder Num [" << rec_num << "]\tTG: " << this->call->get_talkgroup_display() << "\tFreq: " << FormatFreq(chan_freq) << " \tTDMA: " << call->get_phase2_tdma() << "\tSlot: " << call->get_tdma_slot();
    } else {
      recording_started();
    }

 

    if (d_delayopen == false) {
      //wav_sink->open(call->get_filename());
    }

    int offset_amount = (center_freq - chan_freq);
    tune_offset(offset_amount);
    
    if (qpsk_mod) {
      modulation_selector->set_output_index(1);
      qpsk_p25_decode->start(call);
    } else {
      modulation_selector->set_output_index(0);
      fsk4_p25_decode->start(call);
    }
    state = active;
    valve->set_enabled(true);
    modulation_selector->set_enabled(true);


    //wav_sink->set_call(call);
  } else {
    BOOST_LOG_TRIVIAL(error) << "p25conventional_recorder.cc: Trying to Start an already Active Logger!!!";
  }
} 

void p25conventional_recorder::recording_started() {
  BOOST_LOG_TRIVIAL(info) << "\t- Started P25 Recorder Num [" << rec_num << "]\tTG: " << this->call->get_talkgroup_display() << "\tFreq: " << FormatFreq(chan_freq) << " \tTDMA: " << call->get_phase2_tdma() << "\tSlot: " << call->get_tdma_slot();
}

char *p25conventional_recorder::get_filename() {
  this->call->create_filename();
  return this->call->get_filename();
}
