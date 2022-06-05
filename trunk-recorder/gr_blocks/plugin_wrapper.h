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

#ifndef INCLUDED_GR_PLUGIN_WRAPPER_H
#define INCLUDED_GR_PLUGIN_WRAPPER_H

#include <boost/log/trivial.hpp>
#include <functional>
#include <gnuradio/blocks/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace blocks {

typedef std::function<void(int16_t *samples, int sampleCount)> plugin_callback;

/*!
 * \brief Wrapps the plugin functions into a single block.
 * \ingroup audio_blk
 *
 * \details
 * Values must be floats within [-1;1].
 * Check gr_make_plugin_wrapper() for extra info.
 */
class BLOCKS_API plugin_wrapper : virtual public sync_block {
public:
  // gr::blocks::plugin_wrapper::sptr

#if GNURADIO_VERSION < 0x030900
  typedef boost::shared_ptr<plugin_wrapper> sptr;
#else
  typedef std::shared_ptr<plugin_wrapper> sptr;
#endif
};

} /* namespace blocks */
} /* namespace gr */

#endif /* INCLUDED_GR_PLUGIN_WRAPPER_H */
