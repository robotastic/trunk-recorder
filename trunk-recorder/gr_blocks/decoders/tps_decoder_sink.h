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

#ifndef INCLUDED_GR_TPS_DECODER_SINK_H
#define INCLUDED_GR_TPS_DECODER_SINK_H

#include "../decoder_wrapper.h"
#include <boost/log/trivial.hpp>
#include <gnuradio/blocks/api.h>
#include <gnuradio/hier_block2.h>

namespace gr {
namespace blocks {

/*!
 * \brief Decodes Motorola Tactical Public Safety signals.
 * \ingroup audio_blk
 *
 * \details
 * Values must be floats within [-1;1].
 */
class BLOCKS_API tps_decoder_sink : virtual public hier_block2 {
public:
  // gr::blocks::wavfile_sink::sptr

#if GNURADIO_VERSION < 0x030900
  typedef boost::shared_ptr<tps_decoder_sink> sptr;
#else
  typedef std::shared_ptr<tps_decoder_sink> sptr;
#endif

  virtual void set_enabled(bool b){};

  virtual bool get_enabled() { return false; };

  virtual void process_message_queues(void){};
};

} /* namespace blocks */
} /* namespace gr */

#endif /* INCLUDED_GR_TPS_DECODER_SINK_H */
