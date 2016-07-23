
#include "freq_xlating_fft_filter.h"

    void freq_xlating_fft_filter::set_taps(std::vector< float > taps) {
        this->taps = taps;
        this->refresh();
      }
    void freq_xlating_fft_filter::set_center_freq(double center_freq) {
        this->center_freq = center_freq;
        this->refresh();
      }

    void freq_xlating_fft_filter::set_nthreads(int nthreads) {
        this->filter->set_nthreads(nthreads);
      }
    void freq_xlating_fft_filter::declare_sample_delay( double samp_delay) {
        this->filter->declare_sample_delay(samp_delay);
      }


freq_xlating_fft_filter_sptr make_freq_xlating_fft_filter(int decimation, const std::vector< float> &taps, double center_freq, double sampling_freq)
{
	return gnuradio::get_initial_sptr(new freq_xlating_fft_filter(decimation, taps, center_freq, sampling_freq));
}

//  return [ x * cmath.exp(i * phase_inc * 1j) for i,x in enumerate(taps) ]
std::vector< float > freq_xlating_fft_filter::rotate_taps( double phase_inc){
  std::vector< float > rtaps;
  dcomp I = dcomp(0.0, 1.0);

  int i=0;
  for(std::vector<float>::iterator it = this->taps.begin(); it != this->taps.end();it++) {
      float f = *it;
      dcomp result = i * phase_inc * I;
  
      float new_value = f * exp(result.real());
      rtaps.push_back(new_value);
      i++;
  }
  return rtaps;
}


void freq_xlating_fft_filter::refresh() {
  const double pi = M_PI; //boost::math::constants::pi<double>();

std::vector< float > rtaps;

    double phase_inc = (2.0 * pi * this->center_freq) / this->samp_rate;
    rtaps = this->rotate_taps( phase_inc);
    this->filter->set_taps(rtaps);
    this->rotator->set_phase_inc(-1 * this->decim * phase_inc);
}

freq_xlating_fft_filter::freq_xlating_fft_filter(int decim, const std::vector< float > &taps, double center_freq, double samp_rate)
	: gr::hier_block2 ("freq_xlating_fft_filter_ccc",
	                   gr::io_signature::make  (1, 1, sizeof(gr_complex)),
	                   gr::io_signature::make  (1, 1, sizeof(gr_complex)))
{

        this->decim  = decim;
        this->taps        = taps;
        this->center_freq = center_freq;
        this->samp_rate   = samp_rate;

        this->filter =  gr::filter::fft_filter_ccf::make(this->decim, taps);
        this->rotator = gr::blocks::rotator_cc::make(0.0);
        connect(self(),0, filter,0);
        connect(filter,0, rotator, 0);
        connect(rotator,0, self(),0);

        // Refresh
        this->refresh();
}
