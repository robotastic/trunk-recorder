/* -*- c++ -*- */
/* 
 * Copyright 2020 Graham J. Norbury - gnorbury@bondcar.com
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "analog_udp_impl.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <sys/time.h>

namespace gr {
    namespace op25_repeater {

        analog_udp::sptr
            analog_udp::make(const char* options, int debug, int msgq_id, gr::msg_queue::sptr queue)
            {
                return gnuradio::get_initial_sptr
                    (new analog_udp_impl(options, debug, msgq_id, queue));
            }

        /*
         * Our public destructor
         */
        analog_udp_impl::~analog_udp_impl()
        {
        }

        static const int MIN_IN = 1;	// mininum number of input streams
        static const int MAX_IN = 1;	// maximum number of input streams

        /*
         * The private constructor
         */
        analog_udp_impl::analog_udp_impl(const char* options, int debug, int msgq_id, gr::msg_queue::sptr queue)
            : gr::block("analog_udp",
                    gr::io_signature::make (MIN_IN, MAX_IN, sizeof (float)),
                    gr::io_signature::make (0, 0, 0)),
            d_msgq_id(msgq_id),
            d_msg_queue(queue),
            d_audio(options, debug)
        {
        }

        int 
            analog_udp_impl::general_work (int noutput_items,
                    gr_vector_int &ninput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items)
            {
                const float *in = (const float *) input_items[0];

                // buffer and scale the incoming samples from float to S16
                for (int i=0; i<ninput_items[0]; i++) {
                    d_pcm.push_back(static_cast<int16_t>(in[i] * 32768));
                }

                // when enough data is available send to network
                while (d_pcm.size() >= UDP_FRAME_SIZE) {
                    d_audio.send_audio(d_pcm.data() ,UDP_FRAME_SIZE * sizeof(int16_t));
                    d_pcm.erase(d_pcm.begin(), d_pcm.begin() + UDP_FRAME_SIZE);
                }

                consume_each(ninput_items[0]);
                // Tell runtime system how many output items we produced.
                return 0;
            }

        void
        analog_udp_impl::set_debug(int debug)
        {
            d_debug = debug;
            d_audio.set_debug(debug);
        }

    } /* namespace op25_repeater */
} /* namespace gr */
