/* -*- c++ -*- */
/* 
 * Copyright 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017 Max H. Parke KA1RBI 
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
#include "frame_assembler_impl.h"
#include "rx_sync.h"
#include "rx_smartnet.h"
#include "rx_subchannel.h"

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

        void frame_assembler_impl::set_xormask(const char* p) {
            if (d_sync)
                d_sync->set_xormask(p);
        }

        void frame_assembler_impl::set_nac(int nac) {
            if (d_sync)
                d_sync->set_nac(nac);
        }

        void frame_assembler_impl::set_slotid(int slotid) {
            if (d_sync)
                d_sync->set_slot_mask(slotid);
        }

        void frame_assembler_impl::set_slotkey(int key) {
            if (d_sync)
                d_sync->set_slot_key(key);
        }

        void frame_assembler_impl::sync_reset() {
            if (d_sync)
                d_sync->sync_reset();
        }

        void frame_assembler_impl::set_debug(int debug) {
            if (d_sync)
                d_sync->set_debug(debug);
        }

        frame_assembler::sptr
            frame_assembler::make(int sys_num, const char* options, int debug, int msgq_id, gr::msg_queue::sptr queue)
            {
                return gnuradio::get_initial_sptr
                    (new frame_assembler_impl(sys_num, options, debug, msgq_id, queue));
            }

        /*
         * Our public destructor
         */
        frame_assembler_impl::~frame_assembler_impl()
        {
            if (d_sync)
                delete d_sync;
        }

        static const int MIN_IN = 1;	// mininum number of input streams
        static const int MAX_IN = 1;	// maximum number of input streams

        /*
         * The private constructor
         */
        frame_assembler_impl::frame_assembler_impl(int sys_num, const char* options, int debug, int msgq_id, gr::msg_queue::sptr queue)
            : gr::block("frame_assembler",
                    gr::io_signature::make (MIN_IN, MAX_IN, sizeof (char)),
                    gr::io_signature::make (0, 0, 0)),
            d_msgq_id(msgq_id),
            d_msg_queue(queue),
            output_queue(),
            d_sync(NULL)
        {
            if (strcasecmp(options, "smartnet") == 0)
                d_sync = new rx_smartnet(options, debug, msgq_id, queue);
            else if (strcasecmp(options, "subchannel") == 0)
                d_sync = new rx_subchannel(options, debug, msgq_id, queue);
            else
                d_sync = new rx_sync(sys_num, options, debug, msgq_id, queue, output_queue);
        }

        int 
            frame_assembler_impl::general_work (int noutput_items,
                    gr_vector_int &ninput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items)
            {

                const uint8_t *in = (const uint8_t *) input_items[0];

                if (d_sync) {
                    for (int i=0; i<ninput_items[0]; i++) {
                        d_sync->rx_sym(in[i]);
                    }
                }
                consume_each(ninput_items[0]);
                // Tell runtime system how many output items we produced.
                return 0;
            }

    } /* namespace op25_repeater */
} /* namespace gr */
