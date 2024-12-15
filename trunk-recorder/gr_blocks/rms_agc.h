#ifndef INCLUDED_GR_RMS_AGC_H
#define INCLUDED_GR_RMS_AGC_H
#include <gnuradio/block.h>
#include <gnuradio/blocks/api.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/blocks/rms_cf.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/io_signature.h>

#include <gnuradio/blocks/add_const_ff.h>

#if GNURADIO_VERSION < 0x030800
#include <gnuradio/blocks/divide_cc.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#else
#include <gnuradio/blocks/divide.h>
#include <gnuradio/blocks/multiply_const.h>
#endif

class rms_agc;
namespace gr {
namespace blocks {

class BLOCKS_API rms_agc : virtual public gr::hier_block2 {
public:
#if GNURADIO_VERSION < 0x030900
  typedef boost::shared_ptr<rms_agc> sptr;
#else
  typedef std::shared_ptr<rms_agc> sptr;
#endif

  static sptr make(double alpha, double reference);
  rms_agc(double a, double r);
  double alpha;
  double reference;

  gr::blocks::rms_cf::sptr rms_cf;
  gr::blocks::multiply_const_ff::sptr multiply_const;
  gr::blocks::float_to_complex::sptr float_to_complex;
  gr::blocks::divide_cc::sptr divide;
  gr::blocks::add_const_ff::sptr add_const;

  double get_alpha();
  void set_alpha(double a);
  double get_reference();
  void set_reference(double r);
};

} // namespace blocks
} // namespace gr
#endif