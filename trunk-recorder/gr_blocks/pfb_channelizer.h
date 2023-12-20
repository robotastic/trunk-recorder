#ifndef PFB_CHANNELIZER_H
#define PFB_CHANNELIZER_H

#include <boost/log/trivial.hpp>
#include <iomanip>
#include <gnuradio/block.h>
#include <gnuradio/blocks/file_source.h>
#include <gnuradio/blocks/stream_to_vector.h>
#include <gnuradio/blocks/throttle.h>
#include <gnuradio/blocks/vector_map.h>
#include <gnuradio/blocks/vector_to_streams.h>
#include <gnuradio/fft/fft_v.h>
#include <gnuradio/filter/filterbank_vcvcf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/pfb_synthesizer_ccf.h>

#include "../formatter.h"

struct Channelizer_Ouput {
  std::vector<int> channelizer_channels;
  std::vector<int> output_channels;
  int output_channel;
  double freq;
  gr::filter::pfb_synthesizer_ccf::sptr synthesizer;
};

class pfb_channelizer : public gr::hier_block2 {
public:
#if GNURADIO_VERSION < 0x030900
  typedef boost::shared_ptr<pfb_channelizer> sptr;
#else
  typedef std::shared_ptr<pfb_channelizer> sptr;
#endif

  static sptr make(double center, double rate, int n_chans, std::vector<double> channel_freqs, int n_filterbanks = 1, std::vector<float> taps = std::vector<float>(),
                   int atten = 100, float channel_bw = 1.0, float transition_bw = 0.2);

  pfb_channelizer(double center, double rate, int n_chans, std::vector<double> channel_freqs, int n_filterbanks = 1, std::vector<float> taps = std::vector<float>(),
                  int atten = 100, float channel_bw = 1.0, float transition_bw = 0.2);

  int find_channel_id(double freq);
void print_channel_freqs(); 
static void print_channel_freqs(double center, double rate, int n_chans); 
static void print_closest_channel_freqs(double freq, double center, double rate, int n_chans);
private:
  std::vector<float> create_taps(int n_chans, int atten, float channel_bw, float transition_bw);
  //static int find_channel_number(double freq, double center, double rate, int n_chans);
  int find_channel_number(double freq, double center, double rate, int n_chans); 

  double d_rate;
  double d_center;
  int d_n_chans;
  std::vector<float> d_synth_taps;
  std::vector<std::pair<int, double>> d_outchans;
  std::vector<Channelizer_Ouput> d_outputs;
  gr::blocks::stream_to_vector::sptr s2v;
  gr::blocks::vector_map::sptr splitter;
  std::vector<gr::filter::filterbank_vcvcf::sptr> fbs;
  gr::blocks::vector_map::sptr combiner;
  gr::fft::fft_v<gr_complex, true>::sptr fft;
  gr::blocks::vector_map::sptr selector;
  gr::blocks::vector_to_streams::sptr v2ss;
};

#endif