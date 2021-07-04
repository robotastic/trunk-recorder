#ifndef RECORDER_H
#define RECORDER_H

#include <cstdio>
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>

#include <gnuradio/filter/firdes.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/io_signature.h>

#if GNURADIO_VERSION < 0x030800
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/analog/sig_source_f.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/filter/freq_xlating_fir_filter_ccf.h>
#include <gnuradio/filter/rational_resampler_base_ccc.h>
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>
#else
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/blocks/multiply.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/filter/freq_xlating_fir_filter.h>
#include <gnuradio/filter/rational_resampler_base.h>
#endif

#include <gnuradio/analog/quadrature_demod_cf.h>

#include <gnuradio/blocks/file_sink.h>

#include <gnuradio/block.h>
#include <gnuradio/blocks/copy.h>
#include <gnuradio/blocks/null_sink.h>

#include <gnuradio/blocks/head.h>

#include <gnuradio/blocks/file_sink.h>

#include <gr_blocks/nonstop_wavfile_sink.h>
#include <op25_repeater/include/op25_repeater/rx_status.h>

#include "../state.h"

unsigned GCD(unsigned u, unsigned v);
std::vector<float> design_filter(double interpolation, double deci);

class Recorder {

public:
  struct DecimSettings {
    long decim;
    long decim2;
  };

  Recorder(std::string type);
  virtual void tune_offset(double f){};
  virtual void tune_freq(double f){};
  virtual bool start(Call *call){ return false;};
  virtual void stop(){};
  virtual void set_tdma_slot(int slot){};
  virtual double get_freq() { return 0; };
  virtual Source *get_source() { return NULL; };
  virtual Call_Source *get_source_list() { return NULL; };
  int get_num() { return rec_num; };
  virtual long get_source_count() { return 0; };
  virtual long get_wav_hz() { return 8000; };
  virtual long get_talkgroup() { return 0; };
  virtual State get_state() { return INACTIVE; };
  virtual Rx_Status get_rx_status() {
    Rx_Status rx_status = {0, 0, 0};
    return rx_status;
  }
  virtual std::string get_type() { return type; }
  virtual bool is_active() { return false; };
  virtual bool is_analog() { return false; };
  virtual bool is_idle() { return true; };
  virtual double get_current_length() { return 0; };
  virtual double since_last_write() { return 0; };
  virtual void clear(){};
  int rec_num;
  static int rec_counter;
  virtual boost::property_tree::ptree get_stats();
  virtual int get_recording_count() { return recording_count; }
  virtual double get_recording_duration() { return recording_duration; }

  virtual void process_message_queues(void){};
  virtual double get_output_sample_rate(){ return 0;}
  virtual int get_output_channels() { return 1; }

protected:
  int recording_count;
  double recording_duration;
  std::string type;
};

#endif
