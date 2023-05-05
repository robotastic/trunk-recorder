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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vector>
#include <string>

#include "p25_crypt_algs.h"
#include "op25_msg_types.h"

// constructor
p25_crypt_algs::p25_crypt_algs(log_ts& logger, int debug, int msgq_id) :
    logts(logger),
    d_debug(debug),
    d_msgq_id(msgq_id),
    d_fr_type(FT_UNK),
    d_algid(0x80),
    d_keyid(0),
    d_mi{0},
    d_key_iter(d_keys.end()),
    d_adp_position(0) {
}

// destructor
p25_crypt_algs::~p25_crypt_algs() {
}

// remove all stored keys
void p25_crypt_algs::reset(void) {
    d_keys.clear();
}

// add or update a key
void p25_crypt_algs::key(uint16_t keyid, uint8_t algid, const std::vector<uint8_t> &key) {
    if ((keyid == 0) || (algid == 0x80))
        return;

    d_keys[keyid] = key_info(algid, key);
}

// generic entry point to prepare for decryption
bool p25_crypt_algs::prepare(uint8_t algid, uint16_t keyid, frame_type fr_type, uint8_t *MI) {
    bool rc = false;
    d_algid = algid;
    d_keyid = keyid;
    memcpy(d_mi, MI, sizeof(d_mi));

    d_key_iter = d_keys.find(keyid);
    if (d_key_iter == d_keys.end()) {
        if (d_debug >= 10) {
            fprintf(stderr, "%s p25_crypt_algs::prepare: keyid[0x%x] not found\n", logts.get(d_msgq_id), keyid);
        }
        return rc;
    }
    if (d_debug >= 10) {
        fprintf(stderr, "%s p25_crypt_algs::prepare: keyid[0x%x] found\n", logts.get(d_msgq_id), keyid);
    }

    switch (algid) {
        case 0xaa: // ADP RC4
            d_adp_position = 0;
            d_fr_type = fr_type;
            adp_keystream_gen();
            rc = true;
            break;
    
        default:
            break;
    }
    return rc;
}

// generic entry point to perform decryption
bool p25_crypt_algs::process(packed_codeword& PCW) {
    bool rc = false;

    if (d_key_iter == d_keys.end())
        return false;

    switch (d_algid) {
        case 0xaa: // ADP RC4
            rc = adp_process(PCW);
            break;
    
        default:
            break;
    }

    return rc;
}

// ADP RC4 decryption
bool p25_crypt_algs::adp_process(packed_codeword& PCW) {
    bool rc = true;
    size_t offset = 0;

    if (d_key_iter == d_keys.end())
        return false;

    switch (d_fr_type) {
        case FT_LDU1:
            offset = 0;
            break;
        case FT_LDU2:
            offset = 101;
            break;
        case FT_2V:
        case FT_4V:
            offset = 0;
            break;
        default:
            rc = false;
            break;
    }
    if ((d_fr_type == FT_LDU1) || (d_fr_type == FT_LDU2)) {
        //FDMA
        offset += (d_adp_position * 11) + 267 + ((d_adp_position < 8) ? 0 : 2); // voice only; skip LCW and LSD
        d_adp_position = (d_adp_position + 1) % 9;
        for (int j = 0; j < 11; ++j) {
            PCW[j] = adp_keystream[j + offset] ^ PCW[j];
        }
    } else if ((d_fr_type == FT_2V) || (d_fr_type == FT_4V)) {
        //TDMA
        offset += (d_adp_position * 7) + 256;
        d_adp_position = (d_adp_position + 1) % 18;
        for (int j = 0; j < 7; ++j) {
            PCW[j] = adp_keystream[j + offset] ^ PCW[j];
        }
        PCW[6] &= 0x80; // mask everything except the MSB of the final codeword
    }

    return rc;
}

// ADP RC4 helper routine to swap two bytes
void p25_crypt_algs::adp_swap(uint8_t *S, uint32_t i, uint32_t j) {
    uint8_t temp = S[i];
    S[i] = S[j];
    S[j] = temp;
}

// ADP RC4 create keystream using supplied key and message_indicator
void p25_crypt_algs::adp_keystream_gen() {
    uint8_t adp_key[13], S[256], K[256];
    uint32_t i, j, k;

    if (d_key_iter == d_keys.end())
        return;

    // Find key value from keyid and set up to create keystream
    std::vector<uint8_t>::const_iterator kval_iter = d_key_iter->second.key.begin();
    for (i = 0; i < (uint32_t)std::max(5-(int)(d_key_iter->second.key.size()), 0); i++) {
        adp_key[i] = 0;             // pad with leading 0 if supplied key too short 
    }
    for ( ; i < 5; i++) {
        adp_key[i] = *kval_iter++;  // copy up to 5 bytes into key array
    }

    j = 0;
    for (i = 5; i < 13; ++i) {
        adp_key[i] = d_mi[i - 5];   // append MI bytes
    }

    for (i = 0; i < 256; ++i) {
        K[i] = adp_key[i % 13];
    }

    for (i = 0; i < 256; ++i) {
        S[i] = i;
    }

    for (i = 0; i < 256; ++i) {
        j = (j + S[i] + K[i]) & 0xFF;
        adp_swap(S, i, j);
    }

    i = j = 0;

    for (k = 0; k < 469; ++k) {
        i = (i + 1) & 0xFF;
        j = (j + S[i]) & 0xFF;
        adp_swap(S, i, j);
        adp_keystream[k] = S[(S[i] + S[j]) & 0xFF];
    }
}

