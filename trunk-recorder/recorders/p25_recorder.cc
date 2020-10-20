
#include "p25_recorder.h"
#include "../formatter.h"
#include <boost/log/trivial.hpp>

p25_recorder_sptr make_p25_recorder(Source *src) {
  p25_recorder *recorder = new p25_recorder();
  recorder->initialize(src);
  return gnuradio::get_initial_sptr(recorder);
}

void p25_recorder::generate_arb_taps() {

  double arb_size = 32;
  double arb_atten = 100;
  // Create a filter that covers the full bandwidth of the output signal

  // If rate >= 1, we need to prevent images in the output,
  // so we have to filter it to less than half the channel
  // width of 0.5.  If rate < 1, we need to filter to less
  // than half the output signal's bw to avoid aliasing, so
  // the half-band here is 0.5*rate.
  double percent = 0.80;

  if (arb_rate <= 1) {
    double halfband = 0.5 * arb_rate;
    double bw = percent * halfband;
    double tb = (percent / 2.0) * halfband;

    // BOOST_LOG_TRIVIAL(info) << "Arb Rate: " << arb_rate << " Half band: " << halfband << " bw: " << bw << " tb: " <<
    // tb;

    // As we drop the bw factor, the optfir filter has a harder time converging;
    // using the firdes method here for better results.
    arb_taps = gr::filter::firdes::low_pass_2(arb_size, arb_size, bw, tb, arb_atten, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
  } else {
    BOOST_LOG_TRIVIAL(error) << "Something is probably wrong! Resampling rate too low";
    exit(1);
  }
}

p25_recorder::p25_recorder()
    : gr::hier_block2("p25_recorder",
                      gr::io_signature::make(1, 1, sizeof(gr_complex)),
                      gr::io_signature::make(0, 0, sizeof(float))),
      Recorder("P25") {
}

p25_recorder::p25_recorder(std::string type)
    : gr::hier_block2("p25_recorder",
                      gr::io_signature::make(1, 1, sizeof(gr_complex)),
                      gr::io_signature::make(0, 0, sizeof(float))),
      Recorder(type) {
}

p25_recorder::DecimSettings p25_recorder::get_decim(long speed) {
  long s = speed;
  long if_freqs[] = {24000, 25000, 32000};
  DecimSettings decim_settings = {-1, -1};
  for (int i = 0; i < 3; i++) {
    long if_freq = if_freqs[i];
    if (s % if_freq != 0) {
      continue;
    }
    long q = s / if_freq;
    if (q & 1) {
      continue;
    }

    if ((q >= 40) && ((q & 3) == 0)) {
      decim_settings.decim = q / 4;
      decim_settings.decim2 = 4;
    } else {
      decim_settings.decim = q / 2;
      decim_settings.decim2 = 2;
    }
    std::cout << "Decim: " << decim_settings.decim << " Decim2:  " << decim_settings.decim2 << std::endl;
    return decim_settings;
  }
  std::cout << "Nothing found" << std::endl;
  return decim_settings;
}
void p25_recorder::initialize_prefilter() {
  double phase1_channel_rate = phase1_symbol_rate * phase1_samples_per_symbol;
  //double phase2_channel_rate = phase2_symbol_rate * phase2_samples_per_symbol;
  long if_rate = phase1_channel_rate;
  long fa = 0;
  long fb = 0;
  if1 = 0;
  if2 = 0;
  samples_per_symbol = phase1_samples_per_symbol;
  symbol_rate = phase1_symbol_rate;
  system_channel_rate = symbol_rate * samples_per_symbol;

  valve = gr::blocks::copy::make(sizeof(gr_complex));
  valve->set_enabled(false);
  lo = gr::analog::sig_source_c::make(input_rate, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
  mixer = gr::blocks::multiply_cc::make();

  p25_recorder::DecimSettings decim_settings = get_decim(input_rate);
  if (decim_settings.decim != -1) {
    double_decim = true;
    decim = decim_settings.decim;
    if1 = input_rate / decim_settings.decim;
    if2 = if1 / decim_settings.decim2;
    fa = 6250;
    fb = if2 / 2;
    BOOST_LOG_TRIVIAL(info) << "\t P25 Recorder two-stage decimator - Initial decimated rate: " << if1 << " Second decimated rate: " << if2 << " FA: " << fa << " FB: " << fb << " System Rate: " << input_rate;
    bandpass_filter_coeffs = gr::filter::firdes::complex_band_pass(1.0, input_rate, -if1 / 2, if1 / 2, if1 / 2);
    lowpass_filter_coeffs = gr::filter::firdes::low_pass(1.0, if1, (fb + fa) / 2, fb - fa);
    bandpass_filter = gr::filter::fft_filter_ccc::make(decim_settings.decim, bandpass_filter_coeffs);
    lowpass_filter = gr::filter::fft_filter_ccf::make(decim_settings.decim2, lowpass_filter_coeffs);
    resampled_rate = if2;
    bfo = gr::analog::sig_source_c::make(if1, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
  } else {
    double_decim = false;
    BOOST_LOG_TRIVIAL(info) << "\t P25 Recorder single-stage decimator - Initial decimated rate: " << if1 << " Second decimated rate: " << if2 << " Initial Decimation: " << decim << " System Rate: " << input_rate;
    lo = gr::analog::sig_source_c::make(input_rate, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
    lowpass_filter_coeffs = gr::filter::firdes::low_pass(1.0, input_rate, 7250, 1450);
    decim = floor(input_rate / if_rate);
    resampled_rate = input_rate / decim;

    lowpass_filter = gr::filter::fft_filter_ccf::make(decim, lowpass_filter_coeffs);
    resampled_rate = input_rate / decim;
  }

  // Cut-Off Filter
  fa = 6250;
  fb = fa + 625;
  cutoff_filter_coeffs = gr::filter::firdes::low_pass(1.0, if_rate, (fb + fa) / 2, fb - fa);
  cutoff_filter = gr::filter::fft_filter_ccf::make(1.0, cutoff_filter_coeffs);

  // ARB Resampler
  arb_rate = if_rate / resampled_rate;
  generate_arb_taps();
  arb_resampler = gr::filter::pfb_arb_resampler_ccf::make(arb_rate, arb_taps);
  BOOST_LOG_TRIVIAL(info) << "\t P25 Recorder ARB - Initial Rate: " << input_rate << " Resampled Rate: " << resampled_rate << " Initial Decimation: " << decim << " System Rate: " << system_channel_rate << " ARB Rate: " << arb_rate;




  connect(self(), 0, valve, 0);
  if (double_decim) {
    connect(valve, 0, bandpass_filter, 0);
    connect(bandpass_filter, 0, mixer, 0);
    connect(bfo, 0, mixer, 1);
  } else {
    connect(valve, 0, mixer, 0);
    connect(lo, 0, mixer, 1);
  }
  connect(mixer, 0, lowpass_filter, 0);
  connect(lowpass_filter, 0, arb_resampler, 0);
  connect(arb_resampler, 0, cutoff_filter, 0);

}




void p25_recorder::initialize(Source *src) {
  source = src;
  chan_freq = source->get_center();
  center_freq = source->get_center();
  config = source->get_config();
  input_rate = source->get_rate();
  qpsk_mod = true;
  silence_frames = source->get_silence_frames();
  squelch_db = 0;
  
  talkgroup = 0;
  d_phase2_tdma = false;
  rec_num = rec_counter++;
  recording_count = 0;
  recording_duration = 0;

  state = inactive;

  timestamp = time(NULL);
  starttime = time(NULL);

  initialize_prefilter();
  //initialize_p25();

  modulation_selector = gr::blocks::selector::make(sizeof(gr_complex), 0 , 0);
  qpsk_demod = make_p25_recorder_qpsk_demod();
  qpsk_p25_decode = make_p25_recorder_decode(silence_frames );
  fsk4_demod = make_p25_recorder_fsk4_demod();
  fsk4_p25_decode = make_p25_recorder_decode(silence_frames );

  // Squelch DB
  // on a trunked network where you know you will have good signal, a carrier
  // power squelch works well. real FM receviers use a noise squelch, where
  // the received audio is high-passed above the cutoff and then fed to a
  // reverse squelch. If the power is then BELOW a threshold, open the squelch.


    // Non-blocking as we are using squelch_two as a gate.
    squelch = gr::analog::pwr_squelch_cc::make(squelch_db, 0.01, 10, false);
  


  modulation_selector->set_enabled(true);

  connect(cutoff_filter, 0, squelch, 0);
  connect(squelch, 0, modulation_selector, 0);
  connect(modulation_selector, 0, fsk4_demod, 0);
  connect(fsk4_demod,0,fsk4_p25_decode,0);
  connect(modulation_selector, 1, qpsk_demod, 0);
  connect(qpsk_demod,0,qpsk_p25_decode,0);

}

void p25_recorder::switch_tdma(bool phase2) {
  double omega;
  double fmax;
  const double pi = M_PI;

  if (phase2) {
    d_phase2_tdma = true;
    symbol_rate = 6000;
    samples_per_symbol = 4; //5;//4;
  } else {
    d_phase2_tdma = false;
    symbol_rate = 4800;
    samples_per_symbol = 5;
  }

  system_channel_rate = symbol_rate * samples_per_symbol;
  //decim = floor(initial_rate / system_channel_rate);
  //resampled_rate = double(initial_rate) / double(decim);
  arb_rate = (double(system_channel_rate) / resampled_rate);
  BOOST_LOG_TRIVIAL(info) << "\t P25 Recorder - Adjust Channel Resampled Rate: " << resampled_rate << " System Rate: " << system_channel_rate << " ARB Rate: " << arb_rate;

  generate_arb_taps();
  arb_resampler->set_rate(arb_rate);
  arb_resampler->set_taps(arb_taps);
  //op25_frame_assembler->set_phase2_tdma(d_phase2_tdma);
  if (qpsk_mod) {
    qpsk_p25_decode->switch_tdma(phase2);
    qpsk_demod->switch_tdma(phase2);
  }
}

void p25_recorder::set_tdma(bool phase2) {
  if (phase2 != d_phase2_tdma) {
    switch_tdma(phase2);
  }
}

void p25_recorder::clear() {
  //op25_frame_assembler->clear();
}

void p25_recorder::autotune() {
  /*if (!qpsk_mod) {
    gr::message::sptr msg;
    msg = tune_queue->delete_head_nowait();

    if (msg != 0) {
      BOOST_LOG_TRIVIAL(info) << "p25_recorder.cc: Freq:\t" << FormatFreq(chan_freq) << "\t Tune: " << msg->arg1();

      // tune_offset(freq + (msg->arg1()*100));
      tune_queue->flush();
    }
  }*/
}

p25_recorder::~p25_recorder() {}

Source *p25_recorder::get_source() {
  return source;
}

int p25_recorder::get_num() {
  return rec_num;
}

bool p25_recorder::is_active() {
  if (state == active) {
    return true;
  } else {
    return false;
  }
}



bool p25_recorder::is_idle() {
  if (state == active) {
    return !squelch->unmuted();
  }
  return true;
}

double p25_recorder::get_freq() {
  return chan_freq;
}

double p25_recorder::get_current_length() {
  if (qpsk_mod) {
    return qpsk_p25_decode->get_current_length();
  } else {
    return fsk4_p25_decode->get_current_length();
  }
}

int p25_recorder::lastupdate() {
  return time(NULL) - timestamp;
}

long p25_recorder::elapsed() {
  return time(NULL) - starttime;
}

void p25_recorder::tune_freq(double f) {
  chan_freq = f;
  float freq = (center_freq - f);
  tune_offset(freq);
}
void p25_recorder::tune_offset(double f) {

  float freq = static_cast<float>(f);

  if (abs(freq) > ((input_rate / 2) - (if1 / 2))) {
    BOOST_LOG_TRIVIAL(info) << "Tune Offset: Freq exceeds limit: " << abs(freq) << " compared to: " << ((input_rate / 2) - (if1 / 2));
  }
  if (double_decim) {
    bandpass_filter_coeffs = gr::filter::firdes::complex_band_pass(1.0, input_rate, -freq - if1 / 2, -freq + if1 / 2, if1 / 2);
    bandpass_filter->set_taps(bandpass_filter_coeffs);
    float bfz = (static_cast<float>(decim) * -freq) / (float)input_rate;
    bfz = bfz - static_cast<int>(bfz);
    if (bfz < -0.5) {
      bfz = bfz + 1.0;
    }
    if (bfz > 0.5) {
      bfz = bfz - 1.0;
    }
    bfo->set_frequency(-bfz * if1);

  } else {
    lo->set_frequency(freq);
  }

  if (!qpsk_mod) {
    fsk4_demod->reset();
    fsk4_p25_decode->reset_rx_status();
  } else {
    qpsk_demod->reset();
    qpsk_p25_decode->reset_rx_status();
  }
}

State p25_recorder::get_state() {
  return state;
}

Rx_Status p25_recorder::get_rx_status() {
  if (qpsk_mod) {
    return qpsk_p25_decode->get_rx_status();
  } else {
    return fsk4_p25_decode->get_rx_status();
  }
}

/* This is called by wav_sink_delayed_open to get a new filename when samples start to come in */

char *p25_recorder::get_filename() {
  this->call->create_filename();
  return this->call->get_filename();
}

void p25_recorder::stop() {
  if (state == active) {
    if (qpsk_mod) {
      recording_duration += qpsk_p25_decode->get_current_length();
    } else {
      recording_duration += fsk4_p25_decode->get_current_length();
    }
    clear();
    BOOST_LOG_TRIVIAL(info) << "\t- Stopping P25 Recorder Num [" << rec_num << "]\tTG: " << this->call->get_talkgroup_display() << "\tFreq: " << FormatFreq(chan_freq) << " \tTDMA: " << d_phase2_tdma << "\tSlot: " << tdma_slot;

    state = inactive;
    valve->set_enabled(false);
    if (qpsk_mod) {
      qpsk_p25_decode->stop();
      qpsk_p25_decode->reset_rx_status();
    } else {
      fsk4_p25_decode->stop();
      fsk4_p25_decode->reset_rx_status();
    }
  } else {
    BOOST_LOG_TRIVIAL(error) << "p25_recorder.cc: Trying to Stop an Inactive Logger!!!";
  }
}


void p25_recorder::set_tdma_slot(int slot) {
  if (qpsk_mod) {
    qpsk_p25_decode->set_tdma_slot(slot);
  } else {
    fsk4_p25_decode->set_tdma_slot(slot);
  }
  tdma_slot = slot;
}

void p25_recorder::start(Call *call) {
  if (state == inactive) {
    System *system = call->get_system();
    timestamp = time(NULL);
    starttime = time(NULL);

    talkgroup = call->get_talkgroup();
    short_name = call->get_short_name();
    chan_freq = call->get_freq();
    this->call = call;


  squelch_db = system->get_squelch_db();
  squelch->set_threshold(squelch_db);
  qpsk_mod = system->get_qpsk_mod();
    set_tdma(call->get_phase2_tdma());

    if (call->get_phase2_tdma()) {
      if (!qpsk_mod) {
             BOOST_LOG_TRIVIAL(error) << "Error - Modulation is FSK4 but receiving Phase 2 call, this will not work";
      }
      set_tdma_slot(call->get_tdma_slot());

      if (call->get_xor_mask()) {
        qpsk_p25_decode->set_xor_mask(call->get_xor_mask());
      } else {
        BOOST_LOG_TRIVIAL(info) << "Error - can't set XOR Mask for TDMA";
      }
    } else {
      set_tdma_slot(0);
    }

/*
    if (d_delayopen) {
      BOOST_LOG_TRIVIAL(info) << "\t- Listening P25 Recorder Num [" << rec_num << "]\tTG: " << this->call->get_talkgroup_display() << "\tFreq: " << FormatFreq(chan_freq) << " \tTDMA: " << call->get_phase2_tdma() << "\tSlot: " << call->get_tdma_slot();
    } else {
      recording_started();
    }
*/
    
    BOOST_LOG_TRIVIAL(info) << "\t- Starting P25 Recorder Num [" << rec_num << "]\tTG: " << this->call->get_talkgroup_display() << "\tFreq: " << FormatFreq(chan_freq) << " \tTDMA: " << call->get_phase2_tdma() << "\tSlot: " << call->get_tdma_slot() << "\tMod: " << qpsk_mod;

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
    
    recording_count++;
  } else {
    BOOST_LOG_TRIVIAL(error) << "p25_recorder.cc: Trying to Start an already Active Logger!!!";
  }
}
