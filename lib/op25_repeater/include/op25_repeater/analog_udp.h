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


#ifndef INCLUDED_OP25_ANALOG_UDP_H
#define INCLUDED_OP25_ANALOG_UDP_H

#include <op25_repeater/api.h>
#include <gnuradio/block.h>
#include <gnuradio/msg_queue.h>

namespace gr {
    namespace op25_repeater {

        /*!
         * \brief <+description of block+>
         * \ingroup op25_repeater
         *
         */
        class OP25_REPEATER_API analog_udp : virtual public gr::block
        {
            public:
                #if GNURADIO_VERSION < 0x030900
                typedef boost::shared_ptr<analog_udp> sptr;
                #else
                typedef std::shared_ptr<analog_udp> sptr;
                #endif
                /*!
                 * \brief Return a shared_ptr to a new instance of op25_repeater::analog_udp.
                 *
                 * To avoid accidental use of raw pointers, op25_repeater::analog_udp's
                 * constructor is in a private implementation
                 * class. op25_repeater::analog_udp::make is the public interface for
                 * creating new instances.
                 */
                static sptr make(const char* options, int debug, int msgq_id, gr::msg_queue::sptr queue);
                virtual void set_debug(int debug) {}
        };

    } // namespace op25_repeater
} // namespace gr

#endif /* INCLUDED_OP25_REPEATER_ANALOG_UDP_H */

