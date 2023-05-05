// Smartnet Decoder (C) Copyright 2020 Graham J. Norbury
// Inspiration and code fragments from mottrunk.txt and gr-smartnet
// 
// This file is part of OP25
// 
// OP25 is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
// 
// OP25 is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
// License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with OP25; see the file COPYING. If not, write to the Free
// Software Foundation, Inc., 51 Franklin Street, Boston, MA
// 02110-1301, USA.


#include "rx_subchannel.h"
#include "op25_msg_types.h"

namespace gr{
    namespace op25_repeater{

        // constructor
        rx_subchannel::rx_subchannel(const char * options, log_ts& logger, int debug, int msgq_id, gr::msg_queue::sptr queue) :
            d_debug(debug),
            d_msgq_id(msgq_id),
            d_msg_queue(queue),
            d_flag_reg(0),
            logts(logger)
        {
            fprintf(stderr, "%s rx_subchannel::rx_sym: subchannel monitoring enabled\n", logts.get(d_msgq_id));
        }

        // destructor
        rx_subchannel::~rx_subchannel() {
        }

        // symbol receiver and framer
        void rx_subchannel::rx_sym(const uint8_t sym) {
            d_flag_reg = ((d_flag_reg << 1) & 0xffff ) | (sym & 1);

            if (d_flag_reg == 0xffff) { // Channel termination indicated by subchannel streaming '1's. How many is sufficient?
                if (d_debug >= 10) {
                    fprintf(stderr, "%s rx_subchannel::rx_sym: analog end detected\n", logts.get(d_msgq_id));
                }
                d_flag_reg = 0;
                send_end_msg();
            }
        }

        void rx_subchannel::sync_reset(void) {
            d_flag_reg = 0;
        }

        void rx_subchannel::set_debug(int debug) {
            d_debug = debug;
        }

        void rx_subchannel::send_end_msg() {
            std::string msg_str = "";
            if ((d_msgq_id >= 0) && (!d_msg_queue->full_p())) {

                gr::message::sptr msg = gr::message::make_from_string(msg_str, get_msg_type(PROTOCOL_SMARTNET, M_SMARTNET_END_PTT), (d_msgq_id<<1), logts.get_ts());
                if (!d_msg_queue->full_p())
                    d_msg_queue->insert_tail(msg);
            }
        }

    } // end namespace op25_repeater
} // end namespace gr

