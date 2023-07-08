
#include "p25_recorder_impl.h"
#include "../formatter.h"
#include "p25_recorder.h"
#include <boost/log/trivial.hpp>

p25_recorder_sptr make_p25_recorder(Source *src, Recorder_Type type) {
  p25_recorder *recorder = new p25_recorder_impl(src, type);

  return gnuradio::get_initial_sptr(recorder);
}

void p25_recorder_impl::generate_arb_taps() {

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
#if GNURADIO_VERSION < 0x030900
    arb_taps = gr::filter::firdes::low_pass_2(arb_size, arb_size, bw, tb, arb_atten, gr::filter::firdes::WIN_BLACKMAN_HARRIS);
#else
    arb_taps = gr::filter::firdes::low_pass_2(arb_size, arb_size, bw, tb, arb_atten, gr::fft::window::WIN_BLACKMAN_HARRIS);
#endif
  } else {
    BOOST_LOG_TRIVIAL(error) << "Something is probably wrong! Resampling rate too low";
    exit(1);
  }
}

p25_recorder_impl::p25_recorder_impl(Source *src, Recorder_Type type)
    : gr::hier_block2("p25_recorder",
                      gr::io_signature::make(1, 1, sizeof(gr_complex)),
                      gr::io_signature::make(0, 0, sizeof(float))),
      Recorder(type) {
  initialize(src);
}

p25_recorder_impl::DecimSettings p25_recorder_impl::get_decim(long speed) {
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
    BOOST_LOG_TRIVIAL(debug) << "P25 recorder Decim: " << decim_settings.decim << " Decim2:  " << decim_settings.decim2;
    return decim_settings;
  }
  BOOST_LOG_TRIVIAL(error) << "P25 recorder Decim: Nothing found";
  return decim_settings;
}
void p25_recorder_impl::initialize_prefilter() {
  double phase1_channel_rate = phase1_symbol_rate * phase1_samples_per_symbol;
  long if_rate = phase1_channel_rate;
  long fa = 0;
  long fb = 0;
  const float pi = M_PI;
  if1 = 0;
  if2 = 0;

  valve = gr::blocks::copy::make(sizeof(gr_complex));
  valve->set_enabled(false);
  lo = gr::analog::sig_source_c::make(input_rate, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
  mixer = gr::blocks::multiply_cc::make();

  p25_recorder_impl::DecimSettings decim_settings = get_decim(input_rate);
  if (decim_settings.decim != -1) {
    double_decim = true;
    decim = decim_settings.decim;
    if1 = input_rate / decim_settings.decim;
    if2 = if1 / decim_settings.decim2;
    fa = 6250;
    fb = if2 / 2;
    BOOST_LOG_TRIVIAL(info) << "\t P25 Recorder two-stage decimator - Initial decimated rate: " << if1 << " Second decimated rate: " << if2 << " FA: " << fa << " FB: " << fb << " System Rate: " << input_rate;
    bandpass_filter_coeffs = gr::filter::firdes::complex_band_pass(1.0, input_rate, -if1 / 2, if1 / 2, if1 / 2);
    #if GNURADIO_VERSION < 0x030900
        lowpass_filter_coeffs = gr::filter::firdes::low_pass(1.0, if1, (fb + fa) / 2, fb - fa, gr::filter::firdes::WIN_HAMMING);
    #else
        lowpass_filter_coeffs = gr::filter::firdes::low_pass(1.0, if1, (fb + fa) / 2, fb - fa, gr::fft::window::WIN_HAMMING);
    #endif
    bandpass_filter = gr::filter::fft_filter_ccc::make(decim_settings.decim, bandpass_filter_coeffs);
    lowpass_filter = gr::filter::fft_filter_ccf::make(decim_settings.decim2, lowpass_filter_coeffs);
    resampled_rate = if2;
    bfo = gr::analog::sig_source_c::make(if1, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
  } else {
    double_decim = false;
    lo = gr::analog::sig_source_c::make(input_rate, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);

    #if GNURADIO_VERSION < 0x030900
        lowpass_filter_coeffs = gr::filter::firdes::low_pass(1.0, input_rate, 7250, 1450, gr::filter::firdes::WIN_HANN);
    #else
        lowpass_filter_coeffs = gr::filter::firdes::low_pass(1.0, input_rate, 7250, 1450, gr::fft::window::WIN_HANN);
    #endif
    decim = floor(input_rate / if_rate);
    resampled_rate = input_rate / decim;
    lowpass_filter = gr::filter::fft_filter_ccf::make(decim, lowpass_filter_coeffs);
    BOOST_LOG_TRIVIAL(info) << "\t P25 Recorder single-stage decimator - Initial decimated rate: " << if1 << " Second decimated rate: " << if2 << " Initial Decimation: " << decim << " System Rate: " << input_rate;
  }

  // Cut-Off Filter
  fa = 6250;
  fb = fa + 1250;
  #if GNURADIO_VERSION < 0x030900
      cutoff_filter_coeffs = gr::filter::firdes::low_pass(1.0, if_rate, (fb + fa) / 2, fb - fa, gr::filter::firdes::WIN_HANN);
  #else
      cutoff_filter_coeffs = gr::filter::firdes::low_pass(1.0, if_rate, (fb + fa) / 2, fb - fa, gr::fft::window::WIN_HANN);
  #endif
  cutoff_filter = gr::filter::fft_filter_ccf::make(1.0, cutoff_filter_coeffs);

  // ARB Resampler
  arb_rate = if_rate / resampled_rate;
  generate_arb_taps();
  arb_resampler = gr::filter::pfb_arb_resampler_ccf::make(arb_rate, arb_taps);
  double sps = floor(resampled_rate / phase1_symbol_rate);
  double def_excess_bw = 0.2;
  BOOST_LOG_TRIVIAL(info) << "\t P25 Recorder ARB - Initial Rate: " << input_rate << " Resampled Rate: " << resampled_rate << " Initial Decimation: " << decim << " ARB Rate: " << arb_rate << " SPS: " << sps;

   // Squelch DB
  // on a trunked network where you know you will have good signal, a carrier
  // power squelch works well. real FM receviers use a noise squelch, where
  // the received audio is high-passed above the cutoff and then fed to a
  // reverse squelch. If the power is then BELOW a threshold, open the squelch.

  squelch = gr::analog::pwr_squelch_cc::make(squelch_db, 0.0001, 0, true);

  rms_agc = gr::blocks::rms_agc::make(0.45, 0.85);
  //rms_agc = gr::op25_repeater::rmsagc_ff::make(0.45, 0.85);
  fll_band_edge = gr::digital::fll_band_edge_cc::make(sps, def_excess_bw, 2*sps+1, (2.0*pi)/sps/250);  // OP25 has this set to 350 instead of 250


  connect(self(), 0, valve, 0);
  if (double_decim) {
    connect(valve, 0, bandpass_filter, 0);
    connect(bandpass_filter, 0, mixer, 0);
    connect(bfo, 0, mixer, 1);
  } else {
    connect(valve, 0,  mixer, 0);
    connect(lo, 0, mixer, 1);
  }
  connect(mixer, 0,lowpass_filter, 0);
  if (arb_rate == 1.0) {
    connect(lowpass_filter, 0, cutoff_filter, 0);
  } else {
    connect(lowpass_filter, 0, arb_resampler, 0);
    connect(arb_resampler, 0, cutoff_filter, 0);
  }
  connect(cutoff_filter,0, squelch, 0);
  connect(squelch, 0, rms_agc, 0);
  connect(rms_agc,0,  fll_band_edge, 0);
}


void p25_recorder_impl::initialize(Source *src) {
  source = src;
  chan_freq = source->get_center();
  center_freq = source->get_center();
  config = source->get_config();
  d_soft_vocoder = config->soft_vocoder;
  input_rate = source->get_rate();
  qpsk_mod = true;
  silence_frames = source->get_silence_frames();
  squelch_db = 0;
  talkgroup = 0;
  d_phase2_tdma = false;
  rec_num = rec_counter++;
  recording_count = 0;
  recording_duration = 0;

  state = INACTIVE;

  timestamp = time(NULL);
  starttime = time(NULL);

  if (config == NULL) {
    this->set_enable_audio_streaming(false);
  } else {
    this->set_enable_audio_streaming(config->enable_audio_streaming);
  }

  initialize_prefilter();
  // initialize_p25();

  modulation_selector = gr::blocks::selector::make(sizeof(gr_complex), 0, 0);
  qpsk_demod = make_p25_recorder_qpsk_demod();
  qpsk_p25_decode = make_p25_recorder_decode(this, silence_frames, d_soft_vocoder);
  fsk4_demod = make_p25_recorder_fsk4_demod();
  fsk4_p25_decode = make_p25_recorder_decode(this, silence_frames, d_soft_vocoder);

  modulation_selector->set_enabled(true);

  connect(fll_band_edge,0, modulation_selector, 0);
  connect(modulation_selector, 0, fsk4_demod, 0);
  connect(fsk4_demod, 0, fsk4_p25_decode, 0);
  connect(modulation_selector, 1, qpsk_demod, 0);
  connect(qpsk_demod, 0, qpsk_p25_decode, 0);

  
}

void p25_recorder_impl::switch_tdma(bool phase2) {
  double phase1_channel_rate = phase1_symbol_rate * phase1_samples_per_symbol;
  double phase2_channel_rate = phase2_symbol_rate * phase2_samples_per_symbol;

  long if_rate = phase1_channel_rate;

  if (phase2) {
    d_phase2_tdma = true;
    if_rate = phase2_channel_rate;
    fll_band_edge->set_samples_per_symbol(phase2_samples_per_symbol);
  } else {
    d_phase2_tdma = false;
    if_rate = phase1_channel_rate;
    fll_band_edge->set_samples_per_symbol(phase1_samples_per_symbol);
  }

  arb_rate = if_rate / double(resampled_rate);
  generate_arb_taps();
  arb_resampler->set_rate(arb_rate);
  arb_resampler->set_taps(arb_taps);

  if (qpsk_mod) {
    qpsk_p25_decode->switch_tdma(phase2);
    qpsk_demod->switch_tdma(phase2);
  }
}



void p25_recorder_impl::set_tdma(bool phase2) {
  if (phase2 != d_phase2_tdma) {
    switch_tdma(phase2);
  }
}

void p25_recorder_impl::reset_block(gr::basic_block_sptr block) {
  gr::block_detail_sptr detail;
  gr::block_sptr grblock = cast_to_block_sptr(block);
  detail = grblock->detail();
  detail->reset_nitem_counters();
}
void p25_recorder_impl::clear() {
  // This lead to weird SegFaults, but the goal was to clear out buffers inbetween transmissions.
  /*
  if (double_decim) {
    //reset_block(bandpass_filter);
    //reset_block(bfo);
  } else {
  //reset_block(lo);
  }
  reset_block(lowpass_filter);
  reset_block(mixer);

  if (arb_rate != 1.0) {
  reset_block(arb_resampler);
  } 

  reset_block(cutoff_filter);
  reset_block(squelch);
  //reset_block(rms_agc); // RMS AGC cant be made into a basic block
  reset_block(fll_band_edge);
  reset_block(modulation_selector);
 

  //reset_block(qpsk_demod); // bad - Seg Faults
  //reset_block(qpsk_p25_decode); // bad - Seg Faults
  //reset_block(fsk4_demod); // bad - Seg Faults
  //reset_block(fsk4_p25_decode);  // bad - Seg Faults

  */
  qpsk_demod->reset();
  qpsk_p25_decode->reset();
  fsk4_demod->reset();
  fsk4_p25_decode->reset();
}

void p25_recorder_impl::autotune() {
  /*if (!qpsk_mod) {
    gr::message::sptr msg;
    msg = tune_queue->delete_head_nowait();

    if (msg != 0) {
      BOOST_LOG_TRIVIAL(info) << "p25_recorder.cc: Freq:\t" << format_freq(chan_freq) << "\t Tune: " << msg->arg1();

      // tune_offset(freq + (msg->arg1()*100));
      tune_queue->flush();
    }
  }*/
}

int p25_recorder_impl::get_freq_error() {   // get frequency error from FLL and convert to Hz
  const float pi = M_PI;
  double phase1_channel_rate = phase1_symbol_rate * phase1_samples_per_symbol;
  double phase2_channel_rate = phase2_symbol_rate * phase2_samples_per_symbol;
  long if_rate;
  if (d_phase2_tdma) {
    if_rate = phase2_channel_rate;
  } else {
    if_rate = phase1_channel_rate;
  }
        return int((fll_band_edge->get_frequency() / (2*pi)) * if_rate);
}

Source *p25_recorder_impl::get_source() {
  return source;
}

int p25_recorder_impl::get_num() {
  return rec_num;
}

double p25_recorder_impl::since_last_write() {
  if (qpsk_mod) {
    return qpsk_p25_decode->since_last_write();
  } else {
    return fsk4_p25_decode->since_last_write();
  }
}

State p25_recorder_impl::get_state() {
  if (qpsk_mod) {
    return qpsk_p25_decode->get_state();
  } else {
    return fsk4_p25_decode->get_state();
  }
}

bool p25_recorder_impl::is_active() {
  if (state == ACTIVE) {
    return true;
  } else {
    return false;
  }
}

bool p25_recorder_impl::is_squelched() {
  if (state == ACTIVE) {
    return !squelch->unmuted();
  }
  return true;
}
bool p25_recorder_impl::is_idle() {
  if (qpsk_mod) {
    if ((qpsk_p25_decode->get_state() == IDLE) || (qpsk_p25_decode->get_state() == STOPPED)) {
      return true;
    }
  } else {
    if ((fsk4_p25_decode->get_state() == IDLE) || (fsk4_p25_decode->get_state() == STOPPED)) {
      return true;
    }
  }
  return false;
}

double p25_recorder_impl::get_freq() {
  return chan_freq;
}

double p25_recorder_impl::get_current_length() {
  if (qpsk_mod) {
    return qpsk_p25_decode->get_current_length();
  } else {
    return fsk4_p25_decode->get_current_length();
  }
}

int p25_recorder_impl::lastupdate() {
  return time(NULL) - timestamp;
}

long p25_recorder_impl::elapsed() {
  return time(NULL) - starttime;
}

void p25_recorder_impl::tune_freq(double f) {
  chan_freq = f;
  float freq = (center_freq - f);
  tune_offset(freq);
}
void p25_recorder_impl::tune_offset(double f) {

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
  /*
  if (!qpsk_mod) {
    fsk4_demod->reset();
  } else {
    qpsk_demod->reset();
  }*/
}


void p25_recorder_impl::set_source(long src) {
  if (qpsk_mod) {
    return qpsk_p25_decode->set_source(src);
  } else {
    return fsk4_p25_decode->set_source(src);
  }
}

std::vector<Transmission> p25_recorder_impl::get_transmission_list() {
  if (qpsk_mod) {
    return qpsk_p25_decode->get_transmission_list();
  } else {
    return fsk4_p25_decode->get_transmission_list();
  }
}

void p25_recorder_impl::stop() {
  if (state == ACTIVE) {
    if (qpsk_mod) {
      recording_duration += qpsk_p25_decode->get_current_length();
    } else {
      recording_duration += fsk4_p25_decode->get_current_length();
    }

    
    BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << this->call->get_talkgroup_display() << "\tFreq: " << format_freq(chan_freq) << "\t\u001b[33mStopping P25 Recorder Num [" << rec_num << "]\u001b[0m\tTDMA: " << d_phase2_tdma << "\tSlot: " << tdma_slot << "\tHz Error: " << this->get_freq_error();

    state = INACTIVE;
    valve->set_enabled(false);
    clear();
    if (qpsk_mod) {
      qpsk_p25_decode->stop();
    } else {
      fsk4_p25_decode->stop();
    }
  } else {
    BOOST_LOG_TRIVIAL(error) << "p25_recorder.cc: Trying to Stop an Inactive Logger!!!";
  }
}

void p25_recorder_impl::set_tdma_slot(int slot) {
  if (qpsk_mod) {
    qpsk_p25_decode->set_tdma_slot(slot);
  } else {
    fsk4_p25_decode->set_tdma_slot(slot);
  }
  tdma_slot = slot;
}

bool p25_recorder_impl::start(Call *call) {
  if (state == INACTIVE) {
    System *system = call->get_system();
    qpsk_mod = system->get_qpsk_mod();
    set_tdma(call->get_phase2_tdma());
    if (call->get_phase2_tdma()) {
      if (!qpsk_mod) {
        BOOST_LOG_TRIVIAL(error) << "Error - Modulation is FSK4 but receiving Phase 2 call, this will not work";
        return false;
      }
      set_tdma_slot(call->get_tdma_slot());

      if (call->get_xor_mask()) {
        qpsk_p25_decode->set_xor_mask(call->get_xor_mask());
      } else {
        BOOST_LOG_TRIVIAL(info) << "Error - can't set XOR Mask for TDMA";
        return false;
      }
    } else {
      set_tdma_slot(0);
    }

    timestamp = time(NULL);
    starttime = time(NULL);

    talkgroup = call->get_talkgroup();
    short_name = call->get_short_name();
    chan_freq = call->get_freq();
    this->call = call;

    squelch_db = system->get_squelch_db();
    squelch->set_threshold(squelch_db);

    BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << this->call->get_talkgroup_display() << "\tFreq: " << format_freq(chan_freq) << "\t\u001b[32mStarting P25 Recorder Num [" << rec_num << "]\u001b[0m\tTDMA: " << call->get_phase2_tdma() << "\tSlot: " << call->get_tdma_slot() << "\tQPSK: " << qpsk_mod;

    int offset_amount = (center_freq - chan_freq);

    tune_offset(offset_amount);
    if (qpsk_mod) {
      modulation_selector->set_output_index(1);
      qpsk_p25_decode->start(call);
    } else {
      modulation_selector->set_output_index(0);
      fsk4_p25_decode->start(call);
    }
    state = ACTIVE;
    valve->set_enabled(true);
    modulation_selector->set_enabled(true);

    recording_count++;
  } else {
    BOOST_LOG_TRIVIAL(error) << "p25_recorder.cc: Trying to Start an already Active Logger!!!";
    return false;
  }
  return true;
}

int Recorder::rec_counter = 0;
