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

#ifndef INCLUDED_GR_DECODER_WRAPPER_IMPL_H
#define INCLUDED_GR_DECODER_WRAPPER_IMPL_H

#include "decoder_wrapper.h"
#include <boost/log/trivial.hpp>

#include "decoders/signal_decoder_sink.h"
#include "decoders/tps_decoder_sink.h"

namespace gr {
namespace blocks {

class decoder_wrapper_impl : public decoder_wrapper {
private:
  gr::blocks::signal_decoder_sink::sptr d_signal_decoder_sink;
  gr::blocks::tps_decoder_sink::sptr d_tps_decoder_sink;
  decoder_callback d_callback;

public:
#if GNURADIO_VERSION < 0x030900
  typedef boost::shared_ptr<decoder_wrapper_impl> sptr;
#else
  typedef std::shared_ptr<decoder_wrapper_impl> sptr;
#endif
  /*
   * \param filename The .wav file to be opened
   * \param n_channels Number of channels (2 = stereo or I/Q output)
   * \param sample_rate Sample rate [S/s]
   * \param bits_per_sample 16 or 8 bit, default is 16
   */
  static sptr make(unsigned int sample_rate, decoder_callback callback);

  decoder_wrapper_impl(unsigned int sample_rate, decoder_callback callback);
  ~decoder_wrapper_impl();

  void set_mdc_enabled(bool b);
  void set_fsync_enabled(bool b);
  void set_star_enabled(bool b);
  void set_tps_enabled(bool b);

  bool get_mdc_enabled();
  bool get_fsync_enabled();
  bool get_star_enabled();
  bool get_tps_enabled();

  void log_decoder_msg(long unitId, const char *signaling_type, SignalType signal);
  void process_message_queues(void);
};

} /* namespace blocks */
} /* namespace gr */

#endif /* INCLUDED_GR_DECODER_WRAPPER_IMPL_H */
