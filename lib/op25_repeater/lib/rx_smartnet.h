// Smartnet Decoder (C) Copyright 2020 Graham J. Norbury
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

#ifndef INCLUDED_RX_SMARTNET_H
#define INCLUDED_RX_SMARTNET_H

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
#include "check_frame_sync.h"
#include "frame_sync_magics.h"
#include "op25_timer.h"
#include "log_ts.h"

#include "rx_base.h"

namespace gr{
    namespace op25_repeater{

        static const int SMARTNET_SYNC_LENGTH    =  8;
        static const int SMARTNET_FRAME_LENGTH   = 84;
        static const int SMARTNET_PAYLOAD_LENGTH = 76;
        static const int SMARTNET_DATA_LENGTH    = 27;
        static const int SMARTNET_CRC_LENGTH     = 10;
        static const int SMARTNET_ID_XOR         = 0x33C7;
        static const int SMARTNET_CMD_XOR        = 0x32A;
        static const int SMARTNET_ID_INV_XOR     = ~SMARTNET_ID_XOR & 0xffff;
        static const int SMARTNET_CMD_INV_XOR    = ~SMARTNET_CMD_XOR & 0x3ff;

        typedef struct {
            uint16_t address;
            uint8_t  group;
            uint16_t command;
            uint8_t raw_data[6];
        } osw_pkt;

        class rx_smartnet : public rx_base {
            public:
                void rx_sym(const uint8_t sym);
                void sync_reset(void);
                void reset_timer(void);
                void call_end(void) { };
                void crypt_reset(void) { };
                void crypt_key(uint16_t keyid, uint8_t algid, const std::vector<uint8_t> &key) { };
                void set_nac(int nac) { };
                void set_slot_mask(int mask) { };
                void set_slot_key(int mask) { };
                void set_xormask(const char* p) { };
                void set_debug(int debug);
                rx_smartnet(const char * options, log_ts& logger, int debug, int msgq_id, gr::msg_queue::sptr queue);
                ~rx_smartnet();

            private:
                void sync_timeout();
                void cbuf_insert(const uint8_t c);
                void deinterleave(const uint8_t* buf);
                void error_correction();
                bool crc_check();
                void send_msg(const char* buf);

                int d_debug;
                int d_msgq_id;
                gr::msg_queue::sptr d_msg_queue;

                op25_timer sync_timer;
                bool d_in_sync;
                unsigned int d_symbol_count;
                uint8_t d_sync_reg;
                uint8_t d_cbuf[SMARTNET_FRAME_LENGTH * 2];
                uint8_t d_raw_frame[SMARTNET_PAYLOAD_LENGTH];
                uint8_t d_ecc_frame[SMARTNET_DATA_LENGTH + SMARTNET_CRC_LENGTH];
                osw_pkt d_pkt;
                unsigned int d_cbuf_idx;
                int d_rx_count;
                unsigned int d_expires;
                int d_shift_reg;
                std::deque<int16_t> d_output_queue[2];
                log_ts& logts;

        };

    } // end namespace op25_repeater
} // end namespace gr
#endif // INCLUDED_RX_SMARTNET_H
