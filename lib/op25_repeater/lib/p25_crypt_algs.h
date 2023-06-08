/* -*- c++ -*- */
/* 
 * Copyright 2022 Graham J. Norbury
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

#ifndef INCLUDED_OP25_REPEATER_P25_CRYPT_ALGS_H
#define INCLUDED_OP25_REPEATER_P25_CRYPT_ALGS_H

#include <gnuradio/msg_queue.h>
#include <vector>
#include <unordered_map>

#include "log_ts.h"

typedef std::vector<uint8_t> packed_codeword;

struct key_info {
    key_info() : algid(0), key() {}
    key_info(uint8_t a, const std::vector<uint8_t> &k) : algid(a), key(k) {}
    uint8_t algid;
    std::vector<uint8_t> key;
};

enum frame_type { FT_UNK = 0, FT_LDU1, FT_LDU2, FT_2V, FT_4V_0, FT_4V_1, FT_4V_2, FT_4V_3 };
enum protocol_type { PT_UNK = 0, PT_P25_PHASE1, PT_P25_PHASE2 };

class p25_crypt_algs
{
    private:
        log_ts& logts;
        int d_debug;
        int d_msgq_id;
        protocol_type d_pr_type;
        uint8_t d_algid;
        uint16_t d_keyid;
        uint8_t d_mi[9];
        std::unordered_map<uint16_t, key_info> d_keys;
        std::unordered_map<uint16_t, key_info>::const_iterator d_key_iter;
        uint8_t adp_keystream[469];
        uint32_t d_adp_position;

        bool adp_process(packed_codeword& PCW, frame_type fr_type, int voice_subframe);
        void adp_keystream_gen();
        void adp_swap(uint8_t *S, uint32_t i, uint32_t j);

    public:
        p25_crypt_algs(log_ts& logger, int debug, int msgq_id);
        ~p25_crypt_algs();

        void key(uint16_t keyid, uint8_t algid, const std::vector<uint8_t> &key);
        bool prepare(uint8_t algid, uint16_t keyid, protocol_type pr_type, uint8_t *MI);
        bool process(packed_codeword& PCW, frame_type fr_type, int voice_subframe);
        void reset(void);
        inline void set_debug(int debug) {d_debug = debug;}
};

#endif /* INCLUDED_OP25_REPEATER_P25_CRYPT_ALGS_H  */
