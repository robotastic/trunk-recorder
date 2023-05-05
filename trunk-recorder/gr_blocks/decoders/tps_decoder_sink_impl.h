/* -*- c++ -*- */
/*
 * Copyright 2020 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_GR_TPS_DECODER_SINK_IMPL_H
#define INCLUDED_GR_TPS_DECODER_SINK_IMPL_H

#include "tps_decoder_sink.h"
#include <boost/dynamic_bitset.hpp>
#include <boost/log/trivial.hpp>

#include <op25_repeater/fsk4_demod_ff.h>
#include <op25_repeater/fsk4_slicer_fb.h>
#include <op25_repeater/include/op25_repeater/p25_frame_assembler.h>

#include <gnuradio/blocks/copy.h>
#include <gnuradio/message.h>
#include <gnuradio/msg_queue.h>

namespace gr {
namespace blocks {

class tps_decoder_sink_impl : public tps_decoder_sink {
private:
  decoder_callback d_callback;

  gr::op25_repeater::fsk4_demod_ff::sptr fsk4_demod;
  gr::op25_repeater::p25_frame_assembler::sptr op25_frame_assembler;
  gr::op25_repeater::fsk4_slicer_fb::sptr slicer;
  gr::blocks::copy::sptr valve;

  void initialize_p25(void);
  void process_message(gr::message::sptr msg);
  void parse_p25_json(std::string json);
  void decode_mbt_data(unsigned long opcode, boost::dynamic_bitset<> &header, boost::dynamic_bitset<> &mbt_data, unsigned long link_id, unsigned long nac);
  void decode_tsbk(boost::dynamic_bitset<> &tsbk, unsigned long nac);

  unsigned long bitset_shift_mask(boost::dynamic_bitset<> &tsbk, int shift, unsigned long long mask);

  std::string to_hex(const std::string &s, bool upper = false, bool spaced = true);

public:
#if GNURADIO_VERSION < 0x030900
  typedef boost::shared_ptr<tps_decoder_sink_impl> sptr;
#else
  typedef std::shared_ptr<tps_decoder_sink_impl> sptr;
#endif

  gr::msg_queue::sptr rx_queue;

  static sptr make(unsigned int sample_rate, decoder_callback callback);

  tps_decoder_sink_impl(unsigned int sample_rate, decoder_callback callback);

  void set_enabled(bool b);

  bool get_enabled();

  void log_decoder_msg(long unitId, const char *signaling_type, SignalType signal);

  void process_message_queues(void);
};

} /* namespace blocks */
} /* namespace gr */

#endif /* INCLUDED_GR_TPS_DECODER_SINK_IMPL_H */
