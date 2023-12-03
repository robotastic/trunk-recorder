#include "pfb_channelizer.h"


pfb_channelizer::sptr
pfb_channelizer::make(double center,  double rate) {
  return gnuradio::get_initial_sptr(new pfb_channelizer(center, rate));
}

class channelizer_hier_ccf : public gr::hier_block2
{
public:
  channelizer_hier_ccf(double center, double rate, int n_chans, int n_filterbanks = 1, std::vector<float> taps = {},
             int atten = 100, float bw = 1.0, float tb = 0.2, float ripple = 0.1)
    : gr::hier_block2("pfb_channelizer_hier_ccf",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(outchans.size(), outchans.size(), sizeof(gr_complex))),
                    d_center(center),
      d_rate(rate) {
  {
    if (n_filterbanks > n_chans)
      n_filterbanks = n_chans;
    if (outchans.empty())
    {
      for (int i = 0; i < n_chans; i++)
        outchans.push_back(i);
    }

    if (taps.empty())
    {
      taps = create_taps(n_chans, atten, bw, tb, ripple);
    }

    // Make taps for each channel
    std::vector<float> chantaps;
    int extra_taps = std::ceil(1.0 * taps.size() / n_chans) * n_chans - taps.size();
    std::cout << "extra_taps: " << extra_taps << " taps size: " << taps.size() << std::endl;
    
    taps.resize(taps.size() + extra_taps, 0.0);

    for (int i = 0; i < n_chans; i++)
    {
      std::vector<float> reversed_taps(taps.begin() + i, taps.end(), n_chans);
      std::reverse(reversed_taps.begin(), reversed_taps.end());
      chantaps.insert(chantaps.end(), reversed_taps.begin(), reversed_taps.end());
    }

    // Create a mapping to separate out each filterbank (a group of channels to be processed together)
    // And a list of sets of taps for each filterbank.

    std::vector<std::vector<std::pair<int, int>>> splitter_mapping;
    std::vector<std::vector<float>> filterbanktaps;
    int total = 0;
    int low_cpp = n_chans / n_filterbanks;
    int extra = n_chans - low_cpp * n_filterbanks;

    std::vector<int> cpps(n_filterbanks, low_cpp);

    for (int i = 0; i < extra; i++) {
      cpps[i]++;
    }

    for (int cpp : cpps)
    {
      std::vector<std::pair<int, int>> mapping;
      std::vector<float> taps;
      for (int i = total; i < total + cpp; i++)
      {
        mapping.push_back(std::make_pair(0, i));
        taps.insert(taps.end(), chantaps.begin() + i * n_chans, chantaps.begin() + (i + 1) * n_chans);
      }
      splitter_mapping.push_back(mapping);
      filterbanktaps.push_back(taps);
      total += cpp;
    }
    assert(total == n_chans);

    // Split the stream of vectors in n_filterbanks streams of vectors.
    s2v = gr::blocks::stream_to_vector(sizeof(gr_complex), n_chans);
    splitter = gr::blocks::vector_map(sizeof(gr_complex), std::vector<int>{n_chans}, splitter_mapping);

    // Create the filterbanks
    fbs.resize(n_filterbanks);
    for (int i = 0; i < n_filterbanks; i++) {
      fbs[i] = gr::filter::filterbank_vcvcf(filterbanktaps[i]);
    }

    // Combine the streams of vectors back into a single stream of vectors.
    combiner = gr::blocks::vector_map(sizeof(gr_complex), cpps, std::vector<std::vector<std::pair<int, int>>>{{}});

    // Add the final FFT to the channelizer.
    fft = gr::fft::fft_vcc(n_chans, true, std::vector<float>(n_chans, 1.0));
    if (outchans != std::vector<int>(n_chans))
      selector = gr::blocks::vector_map(sizeof(gr_complex), std::vector<int>{n_chans}, std::vector<std::vector<std::pair<int, int>>>{{}});
    v2ss = gr::blocks::vector_to_streams(sizeof(gr_complex), outchans.size());

    connect(self(), s2v, splitter);
    for (int i = 0; i < n_filterbanks; i++) {
      connect(splitter, i, fbs[i], combiner, i);
    }
    connect(combiner, fft);
    if (outchans != std::vector<int>(n_chans)) {
      connect(fft, selector, v2ss);
    }
    else {

      connect(fft, v2ss);
    }
    for (int i = 0; i < outchans.size(); i++) {
      connect(v2ss, i, self(), i);
    }
  }

private:
  gr::blocks::stream_to_vector::sptr s2v;
  gr::blocks::vector_map::sptr splitter;
  std::vector<gr::filter::filterbank_vcvcf::sptr> fbs;
  gr::blocks::vector_map::sptr combiner;
  gr::fft::fft_vcc::sptr fft;
  gr::blocks::vector_map::sptr selector;
  gr::blocks::vector_to_streams::sptr v2ss;

  static std::vector<float> create_taps(int n_chans, int atten = 100, float bw = 1.0, float tb = 0.2, float ripple = 0.1)
  {
    return gr::filter::optfir::low_pass(1, n_chans, bw, bw + tb, ripple, atten);
  }
};


  class channelizer_hier_ccf(gr.hier_block2):
    """
    Make a Polyphase Filter channelizer (complex in, complex out, floating-point taps)

    Args:
    n_chans: The number of channels to split into.
    n_filterbanks: The number of filterbank blocks to use (default=2).
    taps: The taps to use.  If this is `None` then taps are generated using optfir.low_pass.
    outchans: Which channels to output streams for (a list of integers) (default is all channels).
    atten: Stop band attenuation.
    bw: The fraction of the channel you want to keep.
    tb: Transition band with as fraction of channel width.
    ripple: Pass band ripple in dB.
    """

    def __init__(self, n_chans, n_filterbanks=1, taps=None, outchans=None,
                 atten=100, bw=1.0, tb=0.2, ripple=0.1):
        if n_filterbanks > n_chans:
            n_filterbanks = n_chans
        if outchans is None:
            outchans = list(range(n_chans))
        gr.hier_block2.__init__(
            self, "pfb_channelizer_hier_ccf",
            gr.io_signature(1, 1, gr.sizeof_gr_complex),
            gr.io_signature(len(outchans), len(outchans), gr.sizeof_gr_complex))
        if taps is None:
            taps = self.create_taps(n_chans, atten, bw, tb, ripple)
        taps = list(taps)
        extra_taps = int(math.ceil(1.0 * len(taps) / n_chans) *
                         n_chans - len(taps))
        taps = taps + [0] * extra_taps
        # Make taps for each channel


        chantaps = [list(reversed(taps[i: len(taps): n_chans]))
                    for i in range(0, n_chans)]
        # Convert the input stream into a stream of vectors.
        self.s2v = blocks.stream_to_vector(gr.sizeof_gr_complex, n_chans)
        # Create a mapping to separate out each filterbank (a group of channels to be processed together)
        # And a list of sets of taps for each filterbank.
        low_cpp = int(n_chans / n_filterbanks)
        extra = n_chans - low_cpp * n_filterbanks
        cpps = [low_cpp + 1] * extra + [low_cpp] * (n_filterbanks - extra)
        splitter_mapping = []
        filterbanktaps = []
        total = 0
        for cpp in cpps:
            splitter_mapping.append([(0, i)
                                    for i in range(total, total + cpp)])
            filterbanktaps.append(chantaps[total: total + cpp])
            total += cpp
        assert(total == n_chans)
        # Split the stream of vectors in n_filterbanks streams of vectors.
        self.splitter = blocks.vector_map(
            gr.sizeof_gr_complex, [n_chans], splitter_mapping)
        # Create the filterbanks
        self.fbs = [filter.filterbank_vcvcf(taps) for taps in filterbanktaps]
        # Combine the streams of vectors back into a single stream of vectors.
        combiner_mapping = [[]]
        for i, cpp in enumerate(cpps):
            for j in range(cpp):
                combiner_mapping[0].append((i, j))
        self.combiner = blocks.vector_map(
            gr.sizeof_gr_complex, cpps, combiner_mapping)
        # Add the final FFT to the channelizer.
        self.fft = fft.fft_vcc(n_chans, forward=True, window=[1.0] * n_chans)
        # Select the desired channels
        if outchans != list(range(n_chans)):
            selector_mapping = [[(0, i) for i in outchans]]
            self.selector = blocks.vector_map(
                gr.sizeof_gr_complex, [n_chans], selector_mapping)
        # Convert stream of vectors to a normal stream.
        self.v2ss = blocks.vector_to_streams(
            gr.sizeof_gr_complex, len(outchans))
        self.connect(self, self.s2v, self.splitter)
        for i in range(0, n_filterbanks):
            self.connect((self.splitter, i), self.fbs[i], (self.combiner, i))
        self.connect(self.combiner, self.fft)
        if outchans != list(range(n_chans)):
            self.connect(self.fft, self.selector, self.v2ss)
        else:
            self.connect(self.fft, self.v2ss)
        for i in range(0, len(outchans)):
            self.connect((self.v2ss, i), (self, i))

    @staticmethod
    def create_taps(n_chans, atten=100, bw=1.0, tb=0.2, ripple=0.1):
        return optfir.low_pass(1, n_chans, bw, bw + tb, ripple, atten)