#ifndef PFB_CHANNELIZER_H
#define PFB_CHANNELIZER_H

#include <gnuradio/blocks/file_source.h>
#include <gnuradio/blocks/throttle.h>
#include <gnuradio/hier_block2.h>



class pfb_channelizer : public gr::hier_block2 {
public:
#if GNURADIO_VERSION < 0x030900
  typedef boost::shared_ptr<pfb_channelizer> sptr;
#else
  typedef std::shared_ptr<pfb_channelizer> sptr;
#endif

  static sptr make(double center,  double rate);
         

  pfb_channelizer(double center, double rate);


  private:
  double d_rate;
  double d_center;

    gr::blocks::file_source::sptr file_source;
    gr::blocks::throttle::sptr throttle;
};




#endif