#ifndef INCLUDED_GR_XLATING_FFT_FILTER_H
#define INCLUDED_GR_XLATING_FFT_FILTER_H
#include <math.h>

#include <gnuradio/blocks/api.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/fft_filter_ccc.h>
#include <gnuradio/blocks/rotator_cc.h>



class freq_xlating_fft_filter;
typedef boost::shared_ptr<freq_xlating_fft_filter> freq_xlating_fft_filter_sptr;
freq_xlating_fft_filter_sptr make_freq_xlating_fft_filter(int decimation,  std::vector< gr_complex > &taps, double center_freq, double samp_rate);

class freq_xlating_fft_filter : public gr::hier_block2 {

  friend freq_xlating_fft_filter_sptr make_freq_xlating_fft_filter(int decimation,  std::vector< gr_complex > &taps, double center_freq, double samp_rate);

    gr::blocks::rotator_cc::sptr rotator;
    gr::filter::fft_filter_ccc::sptr filter;
    int decim;
    std::vector< gr_complex > taps;
    double center_freq;
    double samp_rate;


    void set_taps(std::vector< gr_complex > taps);
    std::vector< gr_complex> rotate_taps( float phase_inc);
    void refresh();

    ~freq_xlating_fft_filter();

    freq_xlating_fft_filter(int decimation,std::vector< gr_complex > &taps, double center_freq, double sampling_freq);
public:
    void set_center_freq( double center_freq);
    void set_nthreads(int nthreads);
    void declare_sample_delay( double samp_delay);
};

#endif
