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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif // ifdef HAVE_CONFIG_H

#include "decoder_wrapper.h"
#include "decoder_wrapper_impl.h"
#include <gnuradio/io_signature.h>
#include <stdexcept>
#include <climits>
#include <cstring>
#include <cmath>
#include <fcntl.h>
#include <gnuradio/thread/thread.h>
#include <boost/math/special_functions/round.hpp>
#include <stdio.h>

#include "decoders/signal_decoder_sink_impl.h"
#include "decoders/tps_decoder_sink_impl.h"

namespace gr {
    namespace blocks {

        decoder_wrapper_impl::sptr
            decoder_wrapper_impl::make(unsigned int sample_rate, int src_num)
        {
            return gnuradio::get_initial_sptr
            (new decoder_wrapper_impl(sample_rate, src_num));
        }

        decoder_wrapper_impl::decoder_wrapper_impl(unsigned int sample_rate, int src_num)
            : hier_block2("decoder_wrapper_impl",
                io_signature::make(1, 1, sizeof(float)),
                io_signature::make(0, 0, 0))
        {
            d_signal_decoder_sink = gr::blocks::signal_decoder_sink_impl::make(sample_rate);
            d_tps_decoder_sink = gr::blocks::tps_decoder_sink_impl::make(sample_rate, src_num);
            
            connect(self(), 0, d_signal_decoder_sink, 0);
        }

        decoder_wrapper_impl::~decoder_wrapper_impl()
        {
            disconnect(self(), 0, d_signal_decoder_sink, 0);
        }

        void decoder_wrapper_impl::set_mdc_enabled(bool b) { d_signal_decoder_sink->set_mdc_enabled(b); };
        void decoder_wrapper_impl::set_fsync_enabled(bool b) { d_signal_decoder_sink->set_fsync_enabled(b); };
        void decoder_wrapper_impl::set_star_enabled(bool b) { d_signal_decoder_sink->set_star_enabled(b); };
        void decoder_wrapper_impl::set_tps_enabled(bool b) { d_tps_decoder_sink->set_enabled(b); };

        bool decoder_wrapper_impl::get_mdc_enabled() { return d_signal_decoder_sink->get_mdc_enabled(); };
        bool decoder_wrapper_impl::get_fsync_enabled() { return d_signal_decoder_sink->get_fsync_enabled(); };
        bool decoder_wrapper_impl::get_star_enabled() { return d_signal_decoder_sink->get_star_enabled(); };
        bool decoder_wrapper_impl::get_tps_enabled() { return d_tps_decoder_sink->get_enabled(); };

        void decoder_wrapper_impl::log_decoder_msg(long unitId, const char* system_type, bool emergency)
        {
            if (d_current_call == NULL)
            {
                BOOST_LOG_TRIVIAL(error) << "Unable to log: " << system_type << " : " << unitId << ", no current call.";
            }
            else
            {
                BOOST_LOG_TRIVIAL(error) << "Logging " << system_type << " : " << unitId << " to current call.";
                d_current_call->add_signal_source(unitId, system_type, emergency);
            }
        }

        void decoder_wrapper_impl::process_message_queues()
        {
            d_tps_decoder_sink->process_message_queues();
        }

        void decoder_wrapper_impl::set_call(Call* call) {
            d_current_call = call;
            d_signal_decoder_sink->set_call(call);
            d_tps_decoder_sink->set_call(call);
        }
        void decoder_wrapper_impl::end_call() {
            set_call(NULL);
        }
    } /* namespace blocks */
} /* namespace gr */
