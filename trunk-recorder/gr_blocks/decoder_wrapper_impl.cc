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

#include "decoder_wrapper_impl.h"
#include "decoder_wrapper.h"
#include <boost/math/special_functions/round.hpp>
#include <climits>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/thread/thread.h>
#include <stdexcept>
#include <stdio.h>

#include "decoders/signal_decoder_sink_impl.h"
#include "decoders/tps_decoder_sink_impl.h"

namespace gr {
namespace blocks {

decoder_wrapper_impl::sptr
decoder_wrapper_impl::make(unsigned int sample_rate, decoder_callback callback) {
  return gnuradio::get_initial_sptr(new decoder_wrapper_impl(sample_rate, callback));
}

decoder_wrapper_impl::decoder_wrapper_impl(unsigned int sample_rate, decoder_callback callback)
    : hier_block2("decoder_wrapper_impl",
                  io_signature::make(1, 1, sizeof(float)),
                  io_signature::make(0, 0, 0)),
      d_callback(callback) {
  d_signal_decoder_sink = gr::blocks::signal_decoder_sink_impl::make(sample_rate, callback);
  d_tps_decoder_sink = gr::blocks::tps_decoder_sink_impl::make(sample_rate, callback);

  connect(self(), 0, d_signal_decoder_sink, 0);
  connect(self(), 0, d_tps_decoder_sink, 0);
}

decoder_wrapper_impl::~decoder_wrapper_impl() {
  disconnect(self(), 0, d_signal_decoder_sink, 0);
  disconnect(self(), 0, d_tps_decoder_sink, 0);
}

void decoder_wrapper_impl::set_mdc_enabled(bool b) { d_signal_decoder_sink->set_mdc_enabled(b); };
void decoder_wrapper_impl::set_fsync_enabled(bool b) { d_signal_decoder_sink->set_fsync_enabled(b); };
void decoder_wrapper_impl::set_star_enabled(bool b) { d_signal_decoder_sink->set_star_enabled(b); };
void decoder_wrapper_impl::set_tps_enabled(bool b) { d_tps_decoder_sink->set_enabled(b); };

bool decoder_wrapper_impl::get_mdc_enabled() { return d_signal_decoder_sink->get_mdc_enabled(); };
bool decoder_wrapper_impl::get_fsync_enabled() { return d_signal_decoder_sink->get_fsync_enabled(); };
bool decoder_wrapper_impl::get_star_enabled() { return d_signal_decoder_sink->get_star_enabled(); };
bool decoder_wrapper_impl::get_tps_enabled() { return d_tps_decoder_sink->get_enabled(); };

void decoder_wrapper_impl::log_decoder_msg(long unitId, const char *signaling_type, SignalType signal) {
  if (d_callback != NULL) {
    d_callback(unitId, signaling_type, signal);
  }
}

void decoder_wrapper_impl::process_message_queues() {
  d_tps_decoder_sink->process_message_queues();
}
} /* namespace blocks */
} /* namespace gr */
