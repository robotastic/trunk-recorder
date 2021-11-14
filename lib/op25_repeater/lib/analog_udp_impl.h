/* -*- c++ -*- */
/* 
 * Copyright 2020 Graham Norbury - gnorbury@bondcar.com
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

#ifndef INCLUDED_OP25_REPEATER_ANALOG_UDP_IMPL_H
#define INCLUDED_OP25_REPEATER_ANALOG_UDP_IMPL_H

#include <op25_repeater/analog_udp.h>

#include <gnuradio/msg_queue.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <deque>
#include "op25_audio.h"
#include "log_ts.h"

typedef std::deque<uint8_t> dibit_queue;

namespace gr {
    namespace op25_repeater {

        static const int UDP_FRAME_SIZE = 160;
        typedef std::vector<int16_t> pcm_samples;

        class analog_udp_impl : public analog_udp
        {
            private:
                int d_debug;
                int d_msgq_id;
                gr::msg_queue::sptr d_msg_queue;
                op25_audio d_audio;
                pcm_samples d_pcm;
                log_ts logts;

                // internal functions
                void set_debug(int debug);

            public:
                analog_udp_impl(const char* options, int debug, int msgq_id, gr::msg_queue::sptr queue);
                ~analog_udp_impl();

                // Where all the action really happens
                int general_work(int noutput_items,
                        gr_vector_int &ninput_items,
                        gr_vector_const_void_star &input_items,
                        gr_vector_void_star &output_items);
        };

    } // namespace op25_repeater
} // namespace gr

#endif /* INCLUDED_OP25_REPEATER_ANALOG_UDP_IMPL_H */
