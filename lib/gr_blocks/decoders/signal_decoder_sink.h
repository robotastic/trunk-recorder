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

#ifndef INCLUDED_GR_SIGNAL_DECODER_SINK_H
#define INCLUDED_GR_SIGNAL_DECODER_SINK_H

#include "../trunk-recorder/call.h"
#include "../trunk-recorder/call_conventional.h"
#include <boost/log/trivial.hpp>
#include <gnuradio/blocks/api.h>
#include <gnuradio/sync_block.h>
#include "../trunk_ctrl/trunk_worker.h"

namespace gr {
    namespace blocks {

        /*!
         * \brief Connects a gnuradio audio block to non-gnuradio signal decoders.
         * \ingroup audio_blk
         *
         * \details
         * Values must be floats within [-1;1].
         * Check gr_make_signal_decoder_sink() for extra info.
         */
        class BLOCKS_API signal_decoder_sink : virtual public sync_block, public virtual trunk_ctrl::trunk_worker
        {
        public:
            // gr::blocks::wavfile_sink::sptr
            typedef boost::shared_ptr<signal_decoder_sink> sptr;

            virtual void set_mdc_enabled(bool b) {};
            virtual void set_fsync_enabled(bool b) {};
            virtual void set_star_enabled(bool b) {};

            virtual bool get_mdc_enabled() { return false; };
            virtual bool get_fsync_enabled() { return false; };
            virtual bool get_star_enabled() { return false; };

            virtual void set_call(Call* call) {};
            virtual void end_call() {};
        };

    } /* namespace blocks */
} /* namespace gr */

#endif /* INCLUDED_GR_SIGNAL_DECODER_SINK_H */
