#ifndef PFB_CHANNELIZER_H
#define PFB_CHANNELIZER_H

#include <gnuradio/block.h>
#include <gnuradio/blocks/file_source.h>
#include <gnuradio/blocks/throttle.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/blocks/stream_to_vector.h>
#include <gnuradio/blocks/vector_map.h>
#include <gnuradio/filter/filterbank_vcvcf.h>
#include <gnuradio/fft/fft_v.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/blocks/vector_to_streams.h>

class pfb_channelizer : public gr::hier_block2 {
public:
#if GNURADIO_VERSION < 0x030900
  typedef boost::shared_ptr<pfb_channelizer> sptr;
#else
  typedef std::shared_ptr<pfb_channelizer> sptr;
#endif

  static sptr make(double center, double rate, int n_chans, int n_filterbanks, std::vector<float> taps,
             int atten, float channel_bw, float transition_bw);
         

  pfb_channelizer(double center, double rate, int n_chans, int n_filterbanks, std::vector<float> taps,
             int atten, float channel_bw, float transition_bw);


private:
  std::vector<float> create_taps(int n_chans, int atten, float channel_bw, float transition_bw);

  double d_rate;
  double d_center;
private:
  gr::blocks::stream_to_vector::sptr s2v;
  gr::blocks::vector_map::sptr splitter;
  std::vector<gr::filter::filterbank_vcvcf::sptr> fbs;
  gr::blocks::vector_map::sptr combiner;
  gr::fft::fft_v<gr_complex, true>::sptr fft;
  gr::blocks::vector_map::sptr selector;
  gr::blocks::vector_to_streams::sptr v2ss;
};

#endif