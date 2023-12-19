#include "pfb_channelizer.h"

void pfb_channelizer::print_channel_freqs(double center, double rate, int n_chans) {
  BOOST_LOG_TRIVIAL(info) << "Channel Freqs Map" << std::endl;
  int rollover = floor(n_chans / 2) - 1;
  double channel_freq = center;
  for (int i = 0; i < n_chans; i++) {
    BOOST_LOG_TRIVIAL(info) << "[ " << i << " ] Channel Freq: " << std::setprecision(9) << channel_freq << std::endl;
    if (i == rollover) {
      if (n_chans % 2 == 0) {
        channel_freq = center - (rate / 2);
      } else {
        channel_freq = center - (rate / 2) - 6250;
      }
    } else {
      channel_freq += 12500;
    }
  }
}

void pfb_channelizer::print_channel_freqs() {
  print_channel_freqs(d_center, d_rate, d_n_chans);
}
void pfb_channelizer::print_closest_channel_freqs(double freq, double center, double rate, int n_chans) {
  int rollover = floor(n_chans / 2) - 1;
  double channel_freq = center;
  double previous_freq = center;

  for (int i = 0; i < n_chans; i++) {

    if (i == rollover) {
      if (n_chans % 2 == 0) {
        channel_freq = center - (rate / 2);
      } else {
        channel_freq = center - (rate / 2) - 6250;
      }
    } else {
      channel_freq += 12500;
    }
    if (freq > previous_freq && freq < channel_freq) {
      BOOST_LOG_TRIVIAL(info) << "Freq: " << format_freq(freq) << " is between Channels at " << format_freq(previous_freq) << " and " << format_freq(channel_freq) << std::endl;
      BOOST_LOG_TRIVIAL(info) << "Try adding " << freq - previous_freq << "Hz to  the center frequency for this source" << std::endl;
      BOOST_LOG_TRIVIAL(info) << "New center frequency would be: " << std::setprecision(9) << center + (freq - previous_freq) << std::endl;
      break;
    }
    previous_freq = channel_freq;
  }
}

int pfb_channelizer::find_channel_number(double freq, double center, double rate, int n_chans) {
  int channel = -1;
  int rollover = floor(n_chans / 2) - 1;

  double channel_freq = center;
  double previous_freq = center;
  for (int i = 0; i < n_chans; i++) {
    // std::cout << "[ " << i << " ] Channel Freq: " << std::setprecision(9) << channel_freq << std::endl;
    if ((freq == channel_freq) && (i != 0) && (i != rollover) && (i != n_chans - 1)) {
      channel = i;
      BOOST_LOG_TRIVIAL(info) << "Freq: " << format_freq(freq) << " is Channel [ " << i << " ] at " << format_freq(channel_freq) << std::endl;
      break;
    }

    if (i == rollover) {
      if (n_chans % 2 == 0) {
        channel_freq = center - (rate / 2);
      } else {
        channel_freq = center - (rate / 2) - 6250;
      }
    } else {
      channel_freq += 12500;
    }
  }
  return channel;
}

pfb_channelizer::sptr
pfb_channelizer::make(double center, double rate, int n_chans, std::vector<double> channel_freqs, int n_filterbanks, std::vector<float> taps,
                      int atten, float channel_bw, float transition_bw) {
  /*
    std::vector<std::pair<int, double>> outchans;
    for (int i = 0; i < channel_freqs.size(); i++) {
      int channel = find_channel_number(channel_freqs[i], center, rate, n_chans);
      if (channel != -1) {
        outchans.push_back(std::make_pair(channel, channel_freqs[i]));
      } else {
        BOOST_LOG_TRIVIAL(info) << "Building Channelizer - Channel not found for freq: " << format_freq(channel_freqs[i]) << std::endl;
        print_channel_freqs(center, rate, n_chans);
        print_closest_channel_freqs(channel_freqs[i], center, rate, n_chans);
        exit(1);
      }
    }*/
  return gnuradio::get_initial_sptr(new pfb_channelizer(center, rate, n_chans, channel_freqs, n_filterbanks, taps, atten, channel_bw, transition_bw));
}

pfb_channelizer::pfb_channelizer(double center, double rate, int n_chans, std::vector<double> channel_freqs, int n_filterbanks, std::vector<float> taps,
                                 int atten, float channel_bw, float transition_bw)
    : gr::hier_block2("pfb_channelizer_hier_ccf",
                      gr::io_signature::make(1, 1, sizeof(gr_complex)),
                      gr::io_signature::make(channel_freqs.size(), channel_freqs.size(), sizeof(gr_complex))),
      d_center(center),
      d_rate(rate),
      d_n_chans(n_chans) {
  if (n_filterbanks > n_chans)
    n_filterbanks = n_chans;

  if (taps.empty()) {
    taps = create_taps(n_chans, atten, channel_bw, transition_bw);
  }
  long channel_rate = 12500;
  long channelizer_output_channel = 0;
  d_synth_taps = gr::filter::firdes::low_pass_2(3, 3 * channel_rate, channel_rate/2, channel_rate/5, 80);

  for (int i = 0; i < channel_freqs.size(); i++) {
    int channelizer_channel = find_channel_number(channel_freqs[i], center, rate, n_chans);

    if (channelizer_channel != -1) {
      std::vector<int> output_channels = {channelizer_output_channel, channelizer_output_channel + 1, channelizer_output_channel + 2};
      channelizer_output_channel += 3;
      std::vector<int> channelizer_channels = {channelizer_channel - 1, channelizer_channel, channelizer_channel + 1};
      Channelizer_Ouput channelizer_output = {channelizer_channels, output_channels, i, channel_freqs[i], gr::filter::pfb_synthesizer_ccf::make(3, d_synth_taps, false)};
      channelizer_output.synthesizer->set_channel_map({2, 0, 1});
      d_outputs.push_back(channelizer_output);
    } else {
      BOOST_LOG_TRIVIAL(info) << "Building Channelizer - Channel not found for freq: " << format_freq(channel_freqs[i]) << std::endl;
      print_channel_freqs(center, rate, n_chans);
      print_closest_channel_freqs(channel_freqs[i], center, rate, n_chans);
      exit(1);
    }
  }

  std::cout << "Channelizer Output Channels: " << d_outputs.size() << " freqs: " << channel_freqs.size() << std::endl;

  int extra_taps = std::ceil(1.0 * taps.size() / n_chans) * n_chans - taps.size();
  taps.resize(taps.size() + extra_taps, 0);

  std::vector<std::vector<float>> chantaps;
  for (int i = 0; i < n_chans; i++) {
    std::vector<float> taps_for_channel;
    for (int j = i; j < taps.size(); j += n_chans) {
      taps_for_channel.push_back(taps[j]);
    }
    std::reverse(taps_for_channel.begin(), taps_for_channel.end());
    chantaps.push_back(taps_for_channel);
  }

  // Convert the input stream into a stream of vectors.
  gr::blocks::stream_to_vector::sptr s2v = gr::blocks::stream_to_vector::make(sizeof(gr_complex), n_chans);
  // Create a mapping to separate out each filterbank (a group of channels to be processed together)
  // And a list of sets of taps for each filterbank.
  int low_cpp = floor(n_chans / n_filterbanks);
  int extra = n_chans - low_cpp * n_filterbanks;
  std::vector<size_t> cpps;
  for (int i = 0; i < n_filterbanks; i++) {
    if (i < extra) {
      cpps.push_back(low_cpp + 1);
    } else {
      cpps.push_back(low_cpp);
    }
  }

  std::vector<std::vector<std::vector<size_t>>> splitter_mapping;
  std::vector<std::vector<std::vector<float>>> filterbanktaps;
  int total = 0;

  for (int i = 0; i < n_filterbanks; i++) {
    int cpp = cpps[i];
    std::vector<std::vector<size_t>> filterbank_splitter_mapping;
    for (int j = total; j < total + cpp; j++) {
      std::vector<size_t> mapping = {0, j};
      filterbank_splitter_mapping.push_back(mapping);
    }
    splitter_mapping.push_back(filterbank_splitter_mapping);
    std::vector<std::vector<float>> temp_taps;
    for (int j = 0; j < cpp; j++) {
      temp_taps.push_back(chantaps[total + j]);
    }
    filterbanktaps.push_back(temp_taps);
    // filterbanktaps.push_back(std::vector<std::vector<float>>(chantaps.begin() + total, chantaps.begin() + total + cpp));

    total += cpp;
  }
  assert(total == n_chans);

  gr::blocks::vector_map::sptr splitter = gr::blocks::vector_map::make(sizeof(gr_complex), std::vector<size_t>{n_chans}, splitter_mapping);
  std::vector<gr::filter::filterbank_vcvcf::sptr> fbs;
  for (const auto &taps : filterbanktaps) {
    gr::filter::filterbank_vcvcf::sptr filterbank = gr::filter::filterbank_vcvcf::make(taps);
    fbs.push_back(filterbank);
  }

  std::vector<std::vector<std::vector<size_t>>> combiner_mapping{{}};
  for (int i = 0; i < n_filterbanks; i++) {
    int cpp = cpps[i];
    for (int j = 0; j < cpp; j++) {
      std::vector<size_t> mapping = {i, j};
      combiner_mapping[0].push_back(mapping);
    }
  }
  gr::blocks::vector_map::sptr combiner = gr::blocks::vector_map::make(sizeof(gr_complex), cpps, combiner_mapping);

  fft = gr::fft::fft_v<gr_complex, true>::make(n_chans, std::vector<float>(n_chans, 1.0));

  gr::blocks::vector_map::sptr selector;
  // if (outchans != std::vector<int>(n_chans))
  //{
  std::vector<std::vector<std::vector<size_t>>> selector_mapping;
  std::vector<std::vector<size_t>> mapping;
  for (int i = 0; i < d_outputs.size(); i++) {
    std::vector<int> outchans = d_outputs[i].channelizer_channels;
    for (int j = 0; j < outchans.size(); j++) {
      std::vector<size_t> tmp = {0, outchans[j]};
      mapping.push_back(tmp);
    }
  }
  selector_mapping.push_back(mapping);
  std::cout << "Selector Mapping size:  " << mapping.size() << std::endl;
  selector = gr::blocks::vector_map::make(sizeof(gr_complex), std::vector<size_t>{n_chans}, selector_mapping);
  //}
  gr::blocks::vector_to_streams::sptr v2ss = gr::blocks::vector_to_streams::make(sizeof(gr_complex), mapping.size());

  connect(self(), 0, s2v, 0);
  connect(s2v, 0, splitter, 0);
  for (int i = 0; i < n_filterbanks; i++) {
    connect(splitter, i, fbs[i], 0);
    connect(fbs[i], 0, combiner, i);
  }
  connect(combiner, 0, fft, 0);

  connect(fft, 0, selector, 0);
  connect(selector, 0, v2ss, 0);

  /*
  for (int i = 0; i < mapping.size(); i++) {
    connect(v2ss, i, self(), i);
  }*/

  for (int i = 0; i < d_outputs.size(); i++) {
    Channelizer_Ouput output = d_outputs[i];
    connect(v2ss, output.output_channels[0], output.synthesizer, 0);
    connect(v2ss, output.output_channels[1], output.synthesizer, 1);
    connect(v2ss, output.output_channels[2], output.synthesizer, 2);
    connect(output.synthesizer, 0, self(), output.output_channel);
  }
}

int pfb_channelizer::find_channel_id(double freq) {
  int channel = -1;
  for (int i = 0; i < d_outputs.size(); i++) {
    if (d_outputs[i].freq == freq) {
      channel = i;
      break;
    }
  }
  return channel;
}

std::vector<float> pfb_channelizer::create_taps(int n_chans, int atten = 100, float channel_bw = 1.0, float transition_bw = 1.2) {

  return gr::filter::firdes::low_pass_2(1, n_chans, 1.0, 0.2, atten, gr::fft::window::win_type::WIN_HAMMING);

  return gr::filter::firdes::low_pass_2(1, n_chans, channel_bw, transition_bw, atten, gr::fft::window::win_type::WIN_HAMMING);
  // return gr::filter::optfir::low_pass::make(1, n_chans, bw, bw + tb, ripple, atten);
}
