
#include "dmr_recorder.h"
#include "../formatter.h"
#include <boost/log/trivial.hpp>
#include <gr_blocks/plugin_wrapper_impl.h>
#include "../plugin_manager/plugin_manager.h"

dmr_recorder_sptr make_dmr_recorder(Source *src) {
  dmr_recorder *recorder = new dmr_recorder();
  recorder->initialize(src);
  return gnuradio::get_initial_sptr(recorder);
}

void dmr_recorder::generate_arb_taps() {

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

dmr_recorder::dmr_recorder()
    : gr::hier_block2("dmr_recorder",
                      gr::io_signature::make(1, 1, sizeof(gr_complex)),
                      gr::io_signature::make(0, 0, sizeof(float))),
      Recorder("DMR") {
}

dmr_recorder::dmr_recorder(std::string type)
    : gr::hier_block2("dmr_recorder",
                      gr::io_signature::make(1, 1, sizeof(gr_complex)),
                      gr::io_signature::make(0, 0, sizeof(float))),
      Recorder(type) {
}

dmr_recorder::DecimSettings dmr_recorder::get_decim(long speed) {
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
    BOOST_LOG_TRIVIAL(debug) << "DMR recorder Decim: " << decim_settings.decim << " Decim2:  " << decim_settings.decim2;
    return decim_settings;
  }
  BOOST_LOG_TRIVIAL(error) << "DMR recorder Decim: Nothing found";
  return decim_settings;
}
void dmr_recorder::initialize_prefilter() {
  double phase1_channel_rate = phase1_symbol_rate * phase1_samples_per_symbol;
  long if_rate = phase1_channel_rate;
  long fa = 0;
  long fb = 0;
  if1 = 0;
  if2 = 0;

  valve = gr::blocks::copy::make(sizeof(gr_complex));
  valve->set_enabled(false);
  lo = gr::analog::sig_source_c::make(input_rate, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
  mixer = gr::blocks::multiply_cc::make();

  dmr_recorder::DecimSettings decim_settings = get_decim(input_rate);
  if (decim_settings.decim != -1) {
    double_decim = true;
    decim = decim_settings.decim;
    if1 = input_rate / decim_settings.decim;
    if2 = if1 / decim_settings.decim2;
    fa = 6250;
    fb = if2 / 2;
    BOOST_LOG_TRIVIAL(info) << "\t DMR Recorder two-stage decimator - Initial decimated rate: " << if1 << " Second decimated rate: " << if2 << " FA: " << fa << " FB: " << fb << " System Rate: " << input_rate;
    bandpass_filter_coeffs = gr::filter::firdes::complex_band_pass(1.0, input_rate, -if1 / 2, if1 / 2, if1 / 2);
    lowpass_filter_coeffs = gr::filter::firdes::low_pass(1.0, if1, (fb + fa) / 2, fb - fa);
    bandpass_filter = gr::filter::fft_filter_ccc::make(decim_settings.decim, bandpass_filter_coeffs);
    lowpass_filter = gr::filter::fft_filter_ccf::make(decim_settings.decim2, lowpass_filter_coeffs);
    resampled_rate = if2;
    bfo = gr::analog::sig_source_c::make(if1, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
  } else {
    double_decim = false;
    BOOST_LOG_TRIVIAL(info) << "\t DMR Recorder single-stage decimator - Initial decimated rate: " << if1 << " Second decimated rate: " << if2 << " Initial Decimation: " << decim << " System Rate: " << input_rate;
    lo = gr::analog::sig_source_c::make(input_rate, gr::analog::GR_SIN_WAVE, 0, 1.0, 0.0);
    lowpass_filter_coeffs = gr::filter::firdes::low_pass(1.0, input_rate, 7250, 1450);
    decim = floor(input_rate / if_rate);
    resampled_rate = input_rate / decim;
    lowpass_filter = gr::filter::fft_filter_ccf::make(decim, lowpass_filter_coeffs);
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
  BOOST_LOG_TRIVIAL(info) << "\t DMR Recorder ARB - Initial Rate: " << input_rate << " Resampled Rate: " << resampled_rate << " Initial Decimation: " << decim << " ARB Rate: " << arb_rate;

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

void dmr_recorder::initialize(Source *src) {
  source = src;
  chan_freq = source->get_center();
  center_freq = source->get_center();
  config = source->get_config();
  input_rate = source->get_rate();
  silence_frames = source->get_silence_frames();
  squelch_db = 0;

  talkgroup = 0;
  d_phase2_tdma = true;
  rec_num = rec_counter++;
  recording_count = 0;
  recording_duration = 0;

  state = INACTIVE;

  timestamp = time(NULL);
  starttime = time(NULL);

  initialize_prefilter();
  //initialize_p25();

  /* FSK4 Demod */
  const double phase1_channel_rate = phase1_symbol_rate * phase1_samples_per_symbol;
  const double pi = M_PI;

  // FSK4: Phase Loop Lock - can only be Phase 1, so locking at that rate.
  double freq_to_norm_radians = pi / (phase1_channel_rate / 2.0);
  double fc = 0.0;
  double fd = 600.0;
  double pll_demod_gain = 1.0 / (fd * freq_to_norm_radians);
  double samples_per_symbol =  5;
  pll_freq_lock = gr::analog::pll_freqdet_cf::make((phase1_symbol_rate / 2.0 * 1.2) * freq_to_norm_radians, (fc + (3 * fd * 1.9)) * freq_to_norm_radians, (fc + (-3 * fd * 1.9)) * freq_to_norm_radians);
  pll_amp = gr::blocks::multiply_const_ff::make(pll_demod_gain * 1.0);

  //FSK4: noise filter - can only be Phase 1, so locking at that rate.
  	#if GNURADIO_VERSION < 0x030900
  baseband_noise_filter_taps = gr::filter::firdes::low_pass_2(1.0, phase1_channel_rate, phase1_symbol_rate / 2.0 * 1.175, phase1_symbol_rate / 2.0 * 0.125, 20.0, gr::filter::firdes::WIN_KAISER, 6.76);
  #else
  baseband_noise_filter_taps = gr::filter::firdes::low_pass_2(1.0, phase1_channel_rate, phase1_symbol_rate / 2.0 * 1.175, phase1_symbol_rate / 2.0 * 0.125, 20.0, gr::fft::window::WIN_KAISER, 6.76);
  
  #endif
  noise_filter = gr::filter::fft_filter_fff::make(1.0, baseband_noise_filter_taps);

  //FSK4: Symbol Taps
  double symbol_decim = 1;

  for (int i = 0; i < samples_per_symbol; i++) {
    sym_taps.push_back(1.0 / samples_per_symbol);
  }
  sym_filter = gr::filter::fir_filter_fff::make(symbol_decim, sym_taps);

  //FSK4: FSK4 Demod - locked at Phase 1 rates, since it can only be Phase 1
  tune_queue = gr::msg_queue::make(20);
  fsk4_demod = gr::op25_repeater::fsk4_demod_ff::make(tune_queue, phase1_channel_rate, phase1_symbol_rate);

  /* P25 Decode */
    //OP25 Slicer
  const float l[] = {-2.0, 0.0, 2.0, 4.0};
  std::vector<float> slices(l, l + sizeof(l) / sizeof(l[0]));
  slicer = gr::op25_repeater::fsk4_slicer_fb::make(slices);
  wav_sink_slot0 = gr::blocks::transmission_sink::make(1, 8000, 16);
  wav_sink_slot1 = gr::blocks::transmission_sink::make(1, 8000, 16);
  //recorder->initialize(src);
  
  //OP25 Frame Assembler
  traffic_queue = gr::msg_queue::make(2);
  rx_queue = gr::msg_queue::make(100);

  int udp_port = 0;
  int verbosity = 0; // 10 = lots of debug messages
  const char *udp_host = "127.0.0.1";
  bool do_imbe = 1;
  bool do_output = 1;
  bool do_msgq = 0;
  bool do_audio_output = 1;
  bool do_tdma = 1;
  bool do_nocrypt = 1;

  framer = gr::op25_repeater::frame_assembler::make(0,"file:///tmp/out1.raw", verbosity, 1, rx_queue);
  //op25_frame_assembler = gr::op25_repeater::p25_frame_assembler::make(0, silence_frames, udp_host, udp_port, verbosity, do_imbe, do_output, do_msgq, rx_queue, do_audio_output, do_tdma, do_nocrypt);
  levels = gr::blocks::multiply_const_ff::make(1);
  plugin_sink = gr::blocks::plugin_wrapper_impl::make(std::bind(&dmr_recorder::plugin_callback_handler, this, std::placeholders::_1, std::placeholders::_2));



  // Squelch DB
  // on a trunked network where you know you will have good signal, a carrier
  // power squelch works well. real FM receviers use a noise squelch, where
  // the received audio is high-passed above the cutoff and then fed to a
  // reverse squelch. If the power is then BELOW a threshold, open the squelch.

  squelch = gr::analog::pwr_squelch_cc::make(squelch_db, 0.0001, 0, true);



  connect(cutoff_filter, 0, squelch, 0);
  connect(squelch, 0, pll_freq_lock, 0);
  connect(pll_freq_lock, 0, pll_amp, 0);
  connect(pll_amp, 0, noise_filter, 0);
  connect(noise_filter, 0, sym_filter, 0);
  connect(sym_filter, 0, fsk4_demod, 0);
  connect(fsk4_demod, 0, slicer,0);
  connect(slicer, 0, framer, 0);
  connect(framer, 0,  wav_sink_slot0, 0);
  connect(framer, 1, wav_sink_slot1, 0);
}

void dmr_recorder::plugin_callback_handler(int16_t *samples, int sampleCount) {
  //plugman_audio_callback(_recorder, samples, sampleCount);
}

void dmr_recorder::switch_tdma(bool phase2) {
  double phase1_channel_rate = phase1_symbol_rate * phase1_samples_per_symbol;
  long if_rate = phase1_channel_rate;

  if (phase2) {
    d_phase2_tdma = true;
    if_rate = phase1_channel_rate;
  } 

  arb_rate = if_rate / resampled_rate;

  generate_arb_taps();
  arb_resampler->set_rate(arb_rate);
  arb_resampler->set_taps(arb_taps);

  //op25_frame_assembler->set_phase2_tdma(d_phase2_tdma);
}

void dmr_recorder::set_tdma(bool phase2) {
  if (phase2 != d_phase2_tdma) {
    switch_tdma(phase2);
  }
}



dmr_recorder::~dmr_recorder() {}

Source *dmr_recorder::get_source() {
  return source;
}

int dmr_recorder::get_num() {
  return rec_num;
}

double dmr_recorder::since_last_write() {
  time_t now = time(NULL);
  return now - wav_sink_slot0->get_stop_time();
}

State dmr_recorder::get_state() {
  return wav_sink_slot0->get_state();
}

bool dmr_recorder::is_active() {
  if (state == ACTIVE) {
    return true;
  } else {
    return false;
  }
}

bool dmr_recorder::is_squelched() {
  if (state == ACTIVE) {
    return !squelch->unmuted();
  }
  return true;
}
bool dmr_recorder::is_idle() {

    if ((wav_sink_slot0->get_state() == IDLE) || (wav_sink_slot0->get_state() == STOPPED)) {
      return true;
    }
  
  return false;
}

double dmr_recorder::get_freq() {
  return chan_freq;
}

double dmr_recorder::get_current_length() {
    return wav_sink_slot0->total_length_in_seconds();
}

int dmr_recorder::lastupdate() {
  return time(NULL) - timestamp;
}

long dmr_recorder::elapsed() {
  return time(NULL) - starttime;
}

void dmr_recorder::tune_freq(double f) {
  chan_freq = f;
  float freq = (center_freq - f);
  tune_offset(freq);
}
void dmr_recorder::tune_offset(double f) {

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

}

void dmr_recorder::set_record_more_transmissions(bool more) {
  
    return wav_sink_slot0->set_record_more_transmissions(more);
}


bool compareTransmissions(Transmission t1, Transmission t2)
{
    return (t1.start_time < t2.start_time);
}
 

std::vector<Transmission> dmr_recorder::get_transmission_list() {
    std::vector<Transmission> return_list = wav_sink_slot0->get_transmission_list();
    std::vector<Transmission> second_list = wav_sink_slot1->get_transmission_list();
    BOOST_LOG_TRIVIAL(info) << "Slot 0: "  << return_list.size() << " Slot 1: "  << second_list.size();
    return_list.insert( return_list.end(), second_list.begin(), second_list.end() );
    BOOST_LOG_TRIVIAL(info) << "Combined: "  << return_list.size();
    sort(return_list.begin(), return_list.end(), compareTransmissions);
    BOOST_LOG_TRIVIAL(info) << "Sorted: "  << return_list.size();
    return return_list;

}



void dmr_recorder::stop() {
  if (state == ACTIVE) {

      recording_duration += wav_sink_slot0->total_length_in_seconds();

    //BOOST_LOG_TRIVIAL(info) << "[" << this->call->get_short_name() << "]\t\033[0;34m" << this->call->get_call_num() << "C\033[0m\t- Stopping P25 Recorder Num [" << rec_num << "]\tTG: " << this->call->get_talkgroup_display() << "\tFreq: " << format_freq(chan_freq) << " \tTDMA: " << d_phase2_tdma << "\tSlot: " << tdma_slot;

    state = INACTIVE;
    valve->set_enabled(false);
    wav_sink_slot0->stop_recording();
    wav_sink_slot1->stop_recording();
  } else {
    BOOST_LOG_TRIVIAL(error) << "dmr_recorder.cc: Trying to Stop an Inactive Logger!!!";
  }
}

void dmr_recorder::set_tdma_slot(int slot) {
  tdma_slot = slot;
  //op25_frame_assembler->set_slotid(tdma_slot);
}

bool dmr_recorder::start(Call *call) {
  if (state == INACTIVE) {
    System *system = call->get_system();
    set_tdma_slot(0);

    /*if (call->get_xor_mask()) {
      op25_frame_assembler->set_xormask(call->get_xor_mask());
    } else {
      BOOST_LOG_TRIVIAL(info) << "Error - can't set XOR Mask for TDMA";
      return false;
    }*/


    timestamp = time(NULL);
    starttime = time(NULL);

    talkgroup = call->get_talkgroup();
    short_name = call->get_short_name();
    chan_freq = call->get_freq();
    this->call = call;

    squelch_db = system->get_squelch_db();
    squelch->set_threshold(squelch_db);

    BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << this->call->get_talkgroup_display() << "\tFreq: " << format_freq(chan_freq) << "\t\u001b[32mStarting DMR Recorder Num [" << rec_num << "]\u001b[0m\tTDMA: " << call->get_phase2_tdma() << "\tSlot: " << call->get_tdma_slot();

    int offset_amount = (center_freq - chan_freq);

    tune_offset(offset_amount);
    levels->set_k(call->get_system()->get_digital_levels());
    wav_sink_slot0->start_recording(call, 0);
    wav_sink_slot1->start_recording(call, 1);
    state = ACTIVE;
    valve->set_enabled(true);

    recording_count++;
  } else {
    BOOST_LOG_TRIVIAL(error) << "dmr_recorder.cc: Trying to Start an already Active Logger!!!";
    return false;
  }
  return true;
}
