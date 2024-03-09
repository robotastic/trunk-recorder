#ifndef SIGMF_RECORDER_H
#define SIGMF_RECORDER_H

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

#include "../gr_blocks/freq_xlating_fft_filter.h"
#include "recorder.h"

class Source;
class sigmf_recorder;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<sigmf_recorder> sigmf_recorder_sptr;
#else
typedef std::shared_ptr<sigmf_recorder> sigmf_recorder_sptr;
#endif

sigmf_recorder_sptr make_sigmf_recorder(Source *src, Recorder_Type type);
#include "../source.h"

class sigmf_recorder : virtual public gr::hier_block2, virtual public Recorder {

public:
  sigmf_recorder(){};
  virtual ~sigmf_recorder(){};
  virtual bool start(Call *call) = 0;
  virtual void stop() = 0;
  virtual double get_freq() = 0;
  virtual int get_freq_error() = 0;
  virtual int get_num() = 0;
  virtual double get_current_length() = 0;
  virtual void set_enabled(bool enabled) {};
  virtual bool is_enabled() { return false; };
  virtual bool is_active() = 0;
  virtual State get_state() = 0;
  virtual int lastupdate() = 0;
  virtual long elapsed() = 0;
};

#endif
