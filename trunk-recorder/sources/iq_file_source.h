#ifndef IQ_FILE_SOURCE_H
#define IQ_FILE_SOURCE_H

#include <gnuradio/blocks/file_source.h>
#include <gnuradio/blocks/throttle.h>
#include <gnuradio/hier_block2.h>



class iq_file_source : public gr::hier_block2 {
    private:

  std::string d_filename;
    double d_rate;
    bool d_repeat;
    gr::blocks::file_source::sptr file_source;
    gr::blocks::throttle::sptr throttle;

public:
#if GNURADIO_VERSION < 0x030900
  typedef boost::shared_ptr<iq_file_source> sptr;
#else
  typedef std::shared_ptr<iq_file_source> sptr;
#endif
  static sptr make(std::string filename,  double rate, bool repeat);
         

  iq_file_source(std::string filename,  double rate, bool repeat);



};




#endif