//
// DMR Protocol Decoder (C) Copyright 2019 Graham J. Norbury
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

#ifndef INCLUDED_DMR_SLOT_H
#define INCLUDED_DMR_SLOT_H

#include <stdint.h>
#include <vector>

#include "dmr_const.h"
#include "dmr_slot.h"
#include "bptc19696.h"
#include "ezpwd/rs"

typedef std::vector<bool> bit_vector;
typedef std::vector<uint8_t> byte_vector;

static const uint8_t VOICE_LC_HEADER_CRC_MASK[]    = {1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0}; // 0x969696
static const uint8_t TERMINATOR_WITH_LC_CRC_MASK[] = {1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1}; // 0x999999
static const uint8_t PI_HEADER_CRC_MASK[]          = {0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1};                 // 0x6969
static const uint8_t DATA_HEADER_CRC_MASK[]        = {1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0};                 // 0xCCCC
static const uint8_t CSBK_CRC_MASK[]               = {1,0,1,0,0,1,0,1,1,0,1,0,0,1,0,1};                 // 0xA5A5

static const unsigned int SLOT_SIZE                = 264; // size in bits
static const unsigned int PAYLOAD_L                =   0; // starting position in bits
static const unsigned int PAYLOAD_R                = 156;
static const unsigned int SYNC_EMB                 = 108;
static const unsigned int SLOT_L                   =  98;
static const unsigned int SLOT_R                   = 156;

enum lc_type {
	VOICE_LC,
	TERM_LC,
	EMBED_LC
};


class dmr_slot {
public:
	dmr_slot(const int chan, const int debug = 0);
	~dmr_slot();
	inline void set_debug(const int debug) { d_debug = debug; };
	inline uint8_t get_cc() { return 	(d_slot_type[0] << 3) + 
						(d_slot_type[1] << 3) + 
						(d_slot_type[2] << 1) + 
						 d_slot_type[3]; };
	inline uint8_t get_data_type() { return (d_slot_type[4] << 3) + 
						(d_slot_type[5] << 3) + 
						(d_slot_type[6] << 1) + 
						 d_slot_type[7]; };
	void load_slot(const uint8_t slot[]);

private:
	uint8_t d_slot[SLOT_SIZE];	// array of bits comprising the current slot
	bit_vector d_slot_type;
	byte_vector d_lc;
	bool d_lc_valid;
	uint64_t d_type;
	int d_debug;
	int d_chan;
	CBPTC19696 bptc;
	ezpwd::RS<255,252> rs12;	// Reed-Solomon(12,9) for Link Control

	bool decode_slot_type();
	bool decode_csbk(uint8_t* csbk);
	bool decode_vlch(uint8_t* vlch);
	bool decode_tlc(uint8_t* tlc);
	bool decode_lc(uint8_t* lc, lc_type type);
};

#endif /* INCLUDED_DMR_SLOT_H */
