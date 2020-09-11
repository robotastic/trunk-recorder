#ifndef P25_RECORDER_DECODE_H
#define P25_RECORDER_DECODE_H

#include <boost/shared_ptr.hpp>
#include <gnuradio/block.h>
#include <gnuradio/block.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/msg_queue.h>
#include <gnuradio/blocks/short_to_float.h>

#include <op25_repeater/gardner_costas_cc.h>
#include <op25_repeater/fsk4_slicer_fb.h>
#include <op25_repeater/include/op25_repeater/fsk4_demod_ff.h>
#include <op25_repeater/include/op25_repeater/p25_frame_assembler.h>
#include <op25_repeater/include/op25_repeater/rx_status.h>
#include <op25_repeater/vocoder.h>

#if GNURADIO_VERSION < 0x030800
#include <gnuradio/blocks/multiply_const_ff.h>
#else
#include <gnuradio/blocks/multiply_const.h>
#endif

#include <gr_blocks/nonstop_wavfile_sink.h>

class p25_recorder_decode;
typedef boost::shared_ptr<p25_recorder_decode> p25_recorder_decode_sptr;
p25_recorder_decode_sptr make_p25_recorder_decode(gr::blocks::nonstop_wavfile_sink::sptr wav_sink, double digital_levels, int silence_frames);

class p25_recorder_decode : public gr::hier_block2 {
  friend p25_recorder_decode_sptr make_p25_recorder_decode(gr::blocks::nonstop_wavfile_sink::sptr wav_sink, double digital_levels, int silence_frames);

protected:

  virtual void initialize(gr::blocks::nonstop_wavfile_sink::sptr wav_sink, double digital_levels, int silence_frames);
  gr::op25_repeater::p25_frame_assembler::sptr op25_frame_assembler;
    gr::msg_queue::sptr traffic_queue;
  gr::msg_queue::sptr rx_queue;
  gr::op25_repeater::fsk4_slicer_fb::sptr slicer;
    gr::blocks::short_to_float::sptr converter;
      gr::blocks::multiply_const_ff::sptr levels;
public:
  p25_recorder_decode();
  virtual ~p25_recorder_decode();
};
#endif