#include "pfb_channelizer.h"

pfb_channelizer::sptr
pfb_channelizer::make(double center, double rate, int n_chans, int n_filterbanks, std::vector<float> taps,
             int atten, float channel_bw, float transition_bw) {
  return gnuradio::get_initial_sptr(new pfb_channelizer(center, rate, n_chans, n_filterbanks, taps, atten, channel_bw, transition_bw));
}

pfb_channelizer::pfb_channelizer(double center, double rate, int n_chans, int n_filterbanks, std::vector<float> taps,
             int atten, float channel_bw, float transition_bw)
    : gr::hier_block2("pfb_channelizer_hier_ccf",
                      gr::io_signature::make(1, 1, sizeof(gr_complex)),
                      gr::io_signature::make(4, 4, sizeof(gr_complex))),
                      //gr::io_signature::make(n_chans, n_chans, sizeof(gr_complex))),
      d_center(center),
      d_rate(rate) {
  if (n_filterbanks > n_chans)
    n_filterbanks = n_chans;

  std::vector<int> outchans={0,1,2,3};
  /*for (int i = 0; i < n_chans; i++)
  {
    outchans.push_back(i);
  }*/
  if (taps.empty()) {
    taps = create_taps(n_chans, atten, channel_bw, transition_bw);
  }

  std::cout << "Tap Size: " << taps.size() << std::endl;
  std::cout << "Number of Channels: " << n_chans << std::endl;
  int extra_taps = std::ceil(1.0 * taps.size() / n_chans) * n_chans - taps.size();
  std::cout << "Extra Taps: " << extra_taps << std::endl;
  taps.resize(taps.size() + extra_taps, 0);
  std::cout << "Taps + Extra Size: " << taps.size() << std::endl;

  std::vector<std::vector<float>> chantaps;
  for (int i = 0; i < n_chans; i++)
  {
      std::vector<float> taps_for_channel;
      for (int j = i; j < taps.size(); j += n_chans)
      {
          taps_for_channel.push_back(taps[j]);
      }
      std::reverse(taps_for_channel.begin(), taps_for_channel.end());
      chantaps.push_back(taps_for_channel);
  }
  std::cout << "Chan Tap Size: " << chantaps.size() << std::endl;
  // Convert the input stream into a stream of vectors.
  gr::blocks::stream_to_vector::sptr s2v = gr::blocks::stream_to_vector::make(sizeof(gr_complex), n_chans);
  // Create a mapping to separate out each filterbank (a group of channels to be processed together)
  // And a list of sets of taps for each filterbank.
  int low_cpp = floor(n_chans / n_filterbanks);
  std::cout << "Low CPP: " << low_cpp << std::endl;
  int extra = n_chans - low_cpp * n_filterbanks;
  std::cout << "Extra: " << extra << std::endl;
  std::vector<size_t> cpps;
  for (int i = 0; i < n_filterbanks; i++)
  { 
      if (i < extra)
      {
          cpps.push_back(low_cpp + 1);
      }
      else
      {
          cpps.push_back(low_cpp);
      }
  }
  std::cout << "CPPs = Channels per filterbank: " << cpps.size() << std::endl;

  std::vector<std::vector<std::vector<size_t>>> splitter_mapping;
  std::vector<std::vector<std::vector<float>>> filterbanktaps;
  int total = 0;

  for (int i = 0; i < n_filterbanks; i++)
  {
      int cpp = cpps[i];
      std::cout << "CPP value in loop: " << cpp << std::endl;
      std::vector<std::vector<size_t>> filterbank_splitter_mapping;
      for (int j = total; j < total + cpp; j++)
      {
          std::vector<size_t> mapping = {0, j};
          filterbank_splitter_mapping.push_back(mapping);
      }
      splitter_mapping.push_back(filterbank_splitter_mapping);
      std::cout << "New: " << splitter_mapping.size() << std::endl;
      std::vector<std::vector<float>> temp_taps;
      for (int j = 0; j < cpp; j++)
      {
          temp_taps.push_back(chantaps[total + j]);
      }
      filterbanktaps.push_back(temp_taps);
      //filterbanktaps.push_back(std::vector<std::vector<float>>(chantaps.begin() + total, chantaps.begin() + total + cpp));
      std::cout << filterbanktaps.size() << std::endl;
      total += cpp;
  }
  assert(total == n_chans);

  gr::blocks::vector_map::sptr splitter = gr::blocks::vector_map::make(sizeof(gr_complex), std::vector<size_t>{n_chans}, splitter_mapping);
  std::vector<gr::filter::filterbank_vcvcf::sptr> fbs;
  for (const auto &taps : filterbanktaps)
  {
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
  if (outchans != std::vector<int>(n_chans))
  {
      std::vector<std::vector<std::vector<size_t>>> selector_mapping;
      std::vector<std::vector<size_t>> mapping;
      for (int i : outchans)
      {
          std::vector<size_t> tmp={0, i};
          mapping.push_back(tmp);
      }
      selector_mapping.push_back(mapping);
      selector = gr::blocks::vector_map::make(sizeof(gr_complex), std::vector<size_t>{n_chans}, selector_mapping);
  }
  gr::blocks::vector_to_streams::sptr v2ss = gr::blocks::vector_to_streams::make(sizeof(gr_complex), outchans.size());

        connect(self(), 0, s2v, 0);
        connect(s2v, 0, splitter, 0);
        for (int i = 0; i < n_filterbanks; i++)
        {
            connect(splitter, i, fbs[i], 0);
            connect(fbs[i], 0, combiner, i);
        }
        connect(combiner,0, fft, 0);
        if (outchans != std::vector<int>(n_chans))
        {
            connect(fft, 0, selector,0);
            connect(selector,0,  v2ss,0);
        }
        else
        {
            connect(fft,0, v2ss,0);
        }
        std::cout << "Outchans Size: " << outchans.size() << std::endl;
        for (int i = 0; i < outchans.size(); i++)
        {
            connect(v2ss, i, self(), i);
        }
}

std::vector<float> pfb_channelizer::create_taps(int n_chans, int atten = 100, float channel_bw = 1.0, float transition_bw = 1.2) {
    std::cout << "Create Taps" << std::endl;
  return gr::filter::firdes::low_pass_2(1, n_chans, 1.0,0.2, atten, gr::fft::window::win_type::WIN_HAMMING);
  
  return gr::filter::firdes::low_pass_2(1, n_chans, channel_bw, transition_bw, atten, gr::fft::window::win_type::WIN_HAMMING);
  //return gr::filter::optfir::low_pass::make(1, n_chans, bw, bw + tb, ripple, atten);
}
