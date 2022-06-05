#ifndef DEBUG_RECORDER_H
#define DEBUG_RECORDER_H

#define _USE_MATH_DEFINES

#include <cstdio>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>
#if GNURADIO_VERSION < 0x030a00
#include <gnuradio/blocks/udp_sink.h>
#else
#include <gnuradio/network/udp_sink.h>
#endif

#include <gnuradio/hier_block2.h>
#include <gnuradio/io_signature.h>

#include "../gr_blocks/freq_xlating_fft_filter.h"
#include "recorder.h"

class Source;
class debug_recorder;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<debug_recorder> debug_recorder_sptr;
#else
typedef std::shared_ptr<debug_recorder> debug_recorder_sptr;
#endif

debug_recorder_sptr make_debug_recorder(Source *src, std::string address, int port);
#include "../source.h"

class debug_recorder : virtual public gr::hier_block2, virtual public Recorder {
  static debug_recorder_sptr make_debug_recorder(Source *src, std::string address, int port);

public:
  debug_recorder(){};
  virtual ~debug_recorder(){};

  virtual void tune_freq(double f) = 0;
  virtual void tune_offset(double f) = 0;
  virtual bool start(Call *call) = 0;
  virtual void stop() = 0;
  virtual double get_freq() = 0;
  virtual int get_num() = 0;
  virtual double get_current_length() = 0;
  virtual bool is_active() = 0;
  virtual State get_state() = 0;
  virtual int lastupdate() = 0;
  virtual long elapsed() = 0;
  virtual Source *get_source() = 0;
  virtual long get_source_count() = 0;
  virtual Call_Source *get_source_list() = 0;
  virtual void initialize_prefilter() = 0;
  virtual DecimSettings get_decim(long speed) = 0;
  virtual void generate_arb_taps() = 0;
};

#endif
