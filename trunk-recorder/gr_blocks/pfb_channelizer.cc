#include "pfb_channelizer.h"

pfb_channelizer::sptr
pfb_channelizer::make(double center, double rate, int n_chans, int n_filterbanks = 1, std::vector<float> taps = {},
                      int atten = 100, float bw = 1.0, float tb = 0.2, float ripple = 0.1) {
  return gnuradio::get_initial_sptr(new pfb_channelizer(center, rate, n_chans, n_filterbanks, taps, atten, bw, tb, ripple));
}

pfb_channelizer::pfb_channelizer(double center, double rate, int n_chans, int n_filterbanks = 1, std::vector<float> taps = {},
                                 int atten = 100, float bw = 1.0, float tb = 0.2, float ripple = 0.1)
    : gr::hier_block2("pfb_channelizer_hier_ccf",
                      gr::io_signature::make(1, 1, sizeof(gr_complex)),
                      gr::io_signature::make(n_chans, n_chans, sizeof(gr_complex))),
      d_center(center),
      d_rate(rate) {
  if (n_filterbanks > n_chans)
    n_filterbanks = n_chans;

  std::vector<int> outchans;
  for (int i = 0; i < n_chans; i++)
    outchans.push_back(i);

  if (taps.empty())
    taps = create_taps(n_chans, atten, bw, tb, ripple);

  std::vector<float> taps_copy(taps);
  int extra_taps = std::ceil(1.0 * taps_copy.size() / n_chans) * n_chans - taps_copy.size();
  taps_copy.resize(taps_copy.size() + extra_taps, 0);

  std::vector<std::vector<float>> chantaps;
  for (int i = 0; i < n_chans; i++) {
    std::vector<float> taps_subset;
    for (int j = i; j < taps_copy.size(); j += n_chans)
      taps_subset.push_back(taps_copy[j]);
    std::reverse(taps_subset.begin(), taps_subset.end());
    chantaps.push_back(taps_subset);
  }

  gr::blocks::stream_to_vector::sptr s2v = gr::blocks::stream_to_vector::make(sizeof(gr_complex), n_chans);
  std::vector<std::vector<std::pair<int, int>>> splitter_mapping;
  std::vector<std::vector<float>> filterbanktaps;
  int total = 0;

  for (int i = 0; i < n_filterbanks; i++) {
    int cpp = (i < n_chans % n_filterbanks) ? n_chans / n_filterbanks + 1 : n_chans / n_filterbanks;
    std::vector<std::pair<int, int>> channels;
    for (int j = total; j < total + cpp; j++)
      channels.push_back(std::make_pair(0, j));
    splitter_mapping.push_back(channels);
    filterbanktaps.push_back(chantaps[total]);
    total += cpp;
  }
  assert(total == n_chans);

  gr::blocks::vector_map::sptr splitter = gr::blocks::vector_map::make(sizeof(gr_complex), std::vector<size_t>{n_chans}, splitter_mapping);

  std::vector<gr::filter::filterbank_vcvcf::sptr> fbs;
  //for (const auto &taps : filterbanktaps) {
    gr::filter::filterbank_vcvcf::sptr filterbank = gr::filter::filterbank_vcvcf::make(filterbanktaps);   //taps);
    fbs.push_back(filterbank);
  //}

  std::vector<std::vector<std::pair<int, int>>> combiner_mapping{{}};
  for (int i = 0; i < n_filterbanks; i++) {
    int cpp = (i < n_chans % n_filterbanks) ? n_chans / n_filterbanks + 1 : n_chans / n_filterbanks;
    for (int j = 0; j < cpp; j++)
      combiner_mapping[0].push_back(std::make_pair(i, j));
  }
  gr::blocks::vector_map::sptr combiner = gr::blocks::vector_map::make(sizeof(gr_complex), std::vector<int>(cpps.begin(), cpps.end()), combiner_mapping);

  fft::fft_vcc::sptr fft = fft::fft_vcc::make(n_chans, true, std::vector<float>(n_chans, 1.0));

  gr::blocks::vector_map::sptr selector;
  if (outchans != std::vector<int>(n_chans)) {
    std::vector<std::vector<std::pair<int, int>>> selector_mapping;
    for (const auto &i : outchans)
      selector_mapping.push_back(std::vector<std::pair<int, int>>{{0, i}});
    selector = gr::blocks::vector_map::make(sizeof(gr_complex), std::vector<int>{n_chans}, selector_mapping);
  }

  gr::blocks::vector_to_streams::sptr v2ss = gr::blocks::vector_to_streams::make(sizeof(gr_complex), outchans.size());

  this->connect(self(), s2v, splitter);
  for (int i = 0; i < n_filterbanks; i++)
    this->connect(std::make_tuple(splitter, i), fbs[i], std::make_tuple(combiner, i));
  this->connect(combiner, fft);
  if (outchans != std::vector<int>(n_chans))
    this->connect(fft, selector, v2ss);
  else
    this->connect(fft, v2ss);
  for (int i = 0; i < outchans.size(); i++)
    this->connect(std::make_tuple(v2ss, i), std::make_tuple(self(), i));
}

std::vector<float> pfb_channelizer::create_taps(int n_chans, int atten = 100, float bw = 1.0, float tb = 0.2, float ripple = 0.1) {
  return gr::filter::optfir::low_pass::make(1, n_chans, bw, bw + tb, ripple, atten);
}
