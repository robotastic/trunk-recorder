// Type II Subchannel Decoder (C) Copyright 2020 Graham J. Norbury
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

#ifndef INCLUDED_RX_SUBCHANNEL_H
#define INCLUDED_RX_SUBCHANNEL_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <deque>
#include <assert.h>
#include <gnuradio/msg_queue.h>

#include "bit_utils.h"
#include "log_ts.h"

#include "rx_base.h"

namespace gr{
    namespace op25_repeater{

        static const int SUBCHANNEL_SYNC_LENGTH    =  5;
        static const int SUBCHANNEL_FRAME_LENGTH   = 28;
        static const int SUBCHANNEL_PAYLOAD_LENGTH = 23;

        class rx_subchannel : public rx_base {
            public:
                void rx_sym(const uint8_t sym);
                void sync_reset(void);
                void reset_timer(void) { };
                void call_end(void) { };
                void crypt_reset(void) { };
                void crypt_key(uint16_t keyid, uint8_t algid, const std::vector<uint8_t> &key) { };
                void set_nac(int nac) { };
                void set_slot_mask(int mask) { };
                void set_slot_key(int mask) { };
                void set_xormask(const char* p) { };
                void set_debug(int debug);
                rx_subchannel(const char * options, log_ts& logger, int debug, int msgq_id, gr::msg_queue::sptr queue);
                ~rx_subchannel();

            private:
                void send_end_msg();

                int d_debug;
                int d_msgq_id;
                gr::msg_queue::sptr d_msg_queue;

                uint16_t d_flag_reg;
                log_ts& logts;
        };

    } // end namespace op25_repeater
} // end namespace gr
#endif // INCLUDED_RX_SUBCHANNEL_H
