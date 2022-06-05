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

#ifndef INCLUDED_GR_DECODER_WRAPPER_H
#define INCLUDED_GR_DECODER_WRAPPER_H

#include <boost/log/trivial.hpp>
#include <functional>
#include <gnuradio/blocks/api.h>
#include <gnuradio/hier_block2.h>

namespace gr {
namespace blocks {

enum SignalType { Normal = 0,
                  Emergency = 1,
                  EmergencyAck = 2,
                  RadioCheck = 3,
                  RadioCheckAck = 4,
                  RadioStun = 5,
                  RadioStunAck = 6,
                  RadioRevive = 7,
                  RadioReviveAck = 8,
                  NormalPre = 9,
                  EmergencyPre = 10 };

typedef std::function<void(long unitId, const char *signaling_type, SignalType signal)> decoder_callback;

/*!
 * \brief Wrapps the decoder functions into a single block.
 * \ingroup audio_blk
 *
 * \details
 * Values must be floats within [-1;1].
 * Check gr_make_decoder_wrapper() for extra info.
 */
class BLOCKS_API decoder_wrapper : virtual public hier_block2 {
public:
// gr::blocks::decoder_wrapper::sptr
#if GNURADIO_VERSION < 0x030900
  typedef boost::shared_ptr<decoder_wrapper> sptr;
#else
  typedef std::shared_ptr<decoder_wrapper> sptr;
#endif
  virtual void set_mdc_enabled(bool b){};
  virtual void set_fsync_enabled(bool b){};
  virtual void set_star_enabled(bool b){};
  virtual void set_tps_enabled(bool b){};

  virtual bool get_mdc_enabled() { return false; };
  virtual bool get_fsync_enabled() { return false; };
  virtual bool get_star_enabled() { return false; };
  virtual bool get_tps_enabled() { return false; }

  virtual void process_message_queues(void){};
};

} /* namespace blocks */
} /* namespace gr */

#endif /* INCLUDED_GR_DECODER_WRAPPER_H */
