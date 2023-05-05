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


#include "rx_smartnet.h"
#include "op25_msg_types.h"

namespace gr{
    namespace op25_repeater{

        // constructor
        rx_smartnet::rx_smartnet(const char * options, log_ts& logger, int debug, int msgq_id, gr::msg_queue::sptr queue) :
            d_debug(debug),
            d_msgq_id(msgq_id),
            d_msg_queue(queue),
            sync_timer(op25_timer(1000000)),
            d_cbuf_idx(0),
            logts(logger)
        {
            sync_reset();
        }

        // destructor
        rx_smartnet::~rx_smartnet() {
        }

        // symbol receiver and framer
        void rx_smartnet::rx_sym(const uint8_t sym) {
            bool crc_ok;
            bool sync_detected = false;
            d_symbol_count ++;
            d_sync_reg = ((d_sync_reg << 1) & 0xff ) | (sym & 1);
            if ((d_sync_reg ^ (uint8_t)SMARTNET_SYNC_MAGIC) == 0) {
                sync_detected = true;
            }
            cbuf_insert(sym);
            d_rx_count ++;

            if (sync_timer.expired()) {                                                 // Check for timeout
                d_in_sync = false;
                d_rx_count = 0;
                sync_timeout();
                return;
            }

            if (sync_detected && !d_in_sync) {                                          // First sync marks starts data collection
                d_in_sync = true;
                d_rx_count = 0;
                return;
            }

            if (!d_in_sync || (d_in_sync && (d_rx_count < SMARTNET_FRAME_LENGTH))) {    // Collect up to expected data frame length
                return;                                                                 // any early sync sequence is treated as data
            }
            
            if (!sync_detected) {                                                       // Frame must end with sync for to be valid
                if (d_debug >= 10) {
                    fprintf(stderr, "%s SMARTNET sync lost\n", logts.get(d_msgq_id));
                }
                d_in_sync = false;
                d_rx_count = 0;
                return;
            }

            d_rx_count = 0;
            int start_idx = d_cbuf_idx;
            assert (start_idx >= 0);
            uint8_t * symbol_ptr = d_cbuf + start_idx;                                  // Start of data frame in circular buffer

            deinterleave(symbol_ptr);
            error_correction();
            crc_ok = crc_check();
            if (!crc_ok) {                                                              // Discard frames with failed CRC
                if (d_debug >= 10) {
                    fprintf(stderr,"%s SMARTNET crc fail\n", logts.get(d_msgq_id));
                }
                return;
            }

            if ((d_msgq_id < 0) && (d_debug >= 10)) {                                   // Log if no trunking, else trunking can do it
                fprintf(stderr, "%s SMARTNET OSW received: (%05d,%s,0x%03x)\n", logts.get(d_msgq_id), d_pkt.address, ((d_pkt.group) ? "g" : "i"), d_pkt.command);
            }

            send_msg((const char*)d_pkt.raw_data);                                      // Send complete OSW to trunking
            reset_timer();
        }

        // Incoming frame is 76 bits {1,2,3,4,5,6,7,8,9,10,11,12,...,74,75,76}
        // Shuffle into position     {1,20,39,58,2,21,40,59,3,22,41,60,...}
        void rx_smartnet::deinterleave(const uint8_t* buf) {
            for(int k = 0; k < (SMARTNET_PAYLOAD_LENGTH / 4); k++) {
                for(int l = 0; l < 4; l++) {
                    d_raw_frame[k * 4 + l] = buf[k + l * 19];
                }
            }
        }

        // Calculate parity and perform error correction where possible
        void rx_smartnet::error_correction() {
            uint8_t expected[SMARTNET_PAYLOAD_LENGTH];
            uint8_t syndrome[SMARTNET_PAYLOAD_LENGTH];

            //first we calculate the EXPECTED parity bits from the RECEIVED bitstream
            //parity is I[i] ^ I[i-1]
            expected[0] = d_raw_frame[0] & 0x01; //info bit
            expected[1] = d_raw_frame[0] & 0x01; //this is a parity bit, prev bits were 0 so we call x ^ 0 = x
            for(int k = 2; k < SMARTNET_PAYLOAD_LENGTH; k += 2) {
                expected[k] = d_raw_frame[k] & 0x01; //info bit
                expected[k+1] = (d_raw_frame[k] & 0x01) ^ (d_raw_frame[k-2] & 0x01); //parity bit
            }

            for(int k = 0; k < SMARTNET_PAYLOAD_LENGTH; k++) {
                syndrome[k] = expected[k] ^ (d_raw_frame[k] & 0x01); //calculate the syndrome
            }

            for(int k = 0; k < ((SMARTNET_PAYLOAD_LENGTH / 2) - 1); k++) {
                //now we correct the data using the syndrome: if two consecutive
                //parity bits are flipped, you've got a bad previous bit
                if(syndrome[2*k+1] && syndrome[2*k+3]) {
                        d_ecc_frame[k] = ~d_raw_frame[2*k] & 0x01;
                }
                else d_ecc_frame[k] = d_raw_frame[2*k];
            }
        }

        // Validate CRC and break out the useful data
        bool rx_smartnet::crc_check() {
            bool crc_ok = false;
            uint16_t crcaccum = 0x0393;
            uint16_t crcop = 0x036E;
            uint16_t crcgiven;

            // calc expected crc
            for(int j = 0; j < SMARTNET_DATA_LENGTH; j++) {
                if(crcop & 0x01) crcop = (crcop >> 1)^0x0225;
                else crcop >>= 1;
                if (d_ecc_frame[j] & 0x01) crcaccum = crcaccum ^ crcop;
            }

            // load given crc
            crcgiven = 0x0000;
            for(int j = 0; j < SMARTNET_CRC_LENGTH; j++) {
                crcgiven <<= 1;
                //crcgiven += !bool(d_ecc_frame[j + SMARTNET_DATA_LENGTH] & 0x01);
                crcgiven += ~d_ecc_frame[j + SMARTNET_DATA_LENGTH] & 0x01;
            }

            // extract data
            // data bits are inverted in the buffer requiring inverted xor mask
            if (crcgiven == crcaccum) {
                crc_ok = true;
                d_pkt.address = 0;
                for (int j = 0; j < 16; j++)
                    d_pkt.address = (d_pkt.address << 1) + (d_ecc_frame[j] & 0x1);
                d_pkt.address ^= SMARTNET_ID_INV_XOR;  // use inverted xor mask
                d_pkt.group = ~d_ecc_frame[16] & 0x1;
                d_pkt.command = 0;
                for (int j = 17; j < 27; j++)
                    d_pkt.command = (d_pkt.command << 1) + (d_ecc_frame[j] & 0x1);
                d_pkt.command ^= SMARTNET_CMD_INV_XOR; // use inverted xor mask

                d_pkt.raw_data[0] = d_pkt.address >> 8;
                d_pkt.raw_data[1] = d_pkt.address & 0xff;
                d_pkt.raw_data[2] = d_pkt.group;
                d_pkt.raw_data[3] = d_pkt.command >> 8;
                d_pkt.raw_data[4] = d_pkt.command & 0xff;
                d_pkt.raw_data[5] = 0;
            }

            return crc_ok;
        }

        void rx_smartnet::cbuf_insert(const uint8_t c) {
            d_cbuf[d_cbuf_idx] = c;
            d_cbuf[d_cbuf_idx + SMARTNET_FRAME_LENGTH] = c;
            d_cbuf_idx = (d_cbuf_idx + 1) % SMARTNET_FRAME_LENGTH;
        }

        void rx_smartnet::sync_reset(void) {
            d_symbol_count = 0;
            d_cbuf_idx = 0;
            d_rx_count = 0;
            d_shift_reg = 0;
            d_sync_reg = 0;
            d_expires = 0;
            d_in_sync = false;

            // Timers reset
            reset_timer();
        }

        void rx_smartnet::reset_timer(void) {
            sync_timer.reset();
        }

        void rx_smartnet::set_debug(int debug) {
            d_debug = debug;
        }

        void rx_smartnet::sync_timeout()
        {
            if ((d_msgq_id >= 0) && (!d_msg_queue->full_p())) {
                std::string m_buf;
                gr::message::sptr msg;
                msg = gr::message::make_from_string(m_buf, get_msg_type(PROTOCOL_SMARTNET, M_SMARTNET_TIMEOUT), (d_msgq_id << 1), logts.get_ts());
                if (!d_msg_queue->full_p())
                    d_msg_queue->insert_tail(msg);
            }
            if (d_debug >= 10) {
                fprintf(stderr, "%s rx_smartnet::sync_timeout:\n", logts.get(d_msgq_id));
            }
            reset_timer();
        }

        void rx_smartnet::send_msg(const char* buf) {
            std::string msg_str = std::string(buf,5);
            if ((d_msgq_id >= 0) && (!d_msg_queue->full_p())) {

                gr::message::sptr msg = gr::message::make_from_string(msg_str, get_msg_type(PROTOCOL_SMARTNET, M_SMARTNET_OSW), (d_msgq_id<<1), logts.get_ts());
                if (!d_msg_queue->full_p())
                    d_msg_queue->insert_tail(msg);
            }
        }

    } // end namespace op25_repeater
} // end namespace gr

