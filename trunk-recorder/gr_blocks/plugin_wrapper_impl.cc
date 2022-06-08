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

#include "plugin_wrapper_impl.h"
#include "plugin_wrapper.h"
#include <boost/math/special_functions/round.hpp>
#include <climits>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/thread/thread.h>
#include <stdexcept>
#include <stdio.h>

namespace gr {
namespace blocks {

plugin_wrapper_impl::sptr
plugin_wrapper_impl::make(plugin_callback callback) {
  return gnuradio::get_initial_sptr(new plugin_wrapper_impl(callback));
}

plugin_wrapper_impl::plugin_wrapper_impl(plugin_callback callback)
    : sync_block("plugin_wrapper_impl",
                 io_signature::make(1, 1, sizeof(int16_t)),
                 io_signature::make(0, 0, 0)),
      d_callback(callback) {}

int plugin_wrapper_impl::work(int noutput_items, gr_vector_const_void_star &input_items, gr_vector_void_star &output_items) {

  gr::thread::scoped_lock guard(d_mutex); // hold mutex for duration of this

  return dowork(noutput_items, input_items, output_items);
}

int plugin_wrapper_impl::dowork(int noutput_items, gr_vector_const_void_star &input_items, gr_vector_void_star &output_items) {

  if (d_callback != NULL) {
    d_callback((int16_t *)input_items[0], noutput_items);
  } else {
    BOOST_LOG_TRIVIAL(warning) << "plugin_wrapper_impl dropped, no callback setup!";
  }

  return noutput_items;
}

} /* namespace blocks */
} /* namespace gr */