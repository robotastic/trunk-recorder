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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <deque>
#include <errno.h>
#include <unistd.h>

#include "dmr_cai.h"

#include "bit_utils.h"
#include "dmr_const.h"
#include "hamming.h"
#include "crc16.h"

dmr_cai::dmr_cai(int debug) :
	d_debug(debug),
	d_shift_reg(0),
	d_chan(0),
	d_slot{dmr_slot(0, debug), dmr_slot(1, debug)} 
{
	d_cach_sig.clear();
	memset(d_frame, 0, sizeof(d_frame));
}

dmr_cai::~dmr_cai() {
}

int
dmr_cai::load_frame(const uint8_t fr_sym[]) {
	dibits_to_bits(d_frame, fr_sym, FRAME_SIZE >> 1);
	extract_cach_fragment();
	d_slot[d_chan].load_slot(d_frame + 24);
	return d_chan;
}

void
dmr_cai::extract_cach_fragment() {
	static const int slot_ids[] = {0, 1, 0, 0, 1, 1, 0, 1};
	int tact, tact_at, tact_tc, tact_lcss;
	uint8_t tactbuf[sizeof(cach_tact_bits)];

	for (size_t i=0; i<sizeof(cach_tact_bits); i++)
		tactbuf[i] = d_frame[CACH + cach_tact_bits[i]];
	tact = hamming_7_4_decode[load_i(tactbuf, 7)];
	tact_at   = (tact>>3) & 1; // Access Type
	tact_tc   = (tact>>2) & 1; // TDMA Channel
	tact_lcss = tact & 3;      // Link Control Start/Stop
	d_shift_reg = (d_shift_reg << 1) + tact_tc;
	d_chan = slot_ids[d_shift_reg & 7];

	switch(tact_lcss) {
		case 0: // Single-fragment CSBK
			// TODO: do something useful
			break;
		case 1: // Begin Short_LC
			d_cach_sig.clear();
			for (size_t i=0; i<sizeof(cach_payload_bits); i++)
				d_cach_sig.push_back(d_frame[CACH + cach_payload_bits[i]]);
			break;
		case 2: // End Short_LC
			for (size_t i=0; i<sizeof(cach_payload_bits); i++)
				d_cach_sig.push_back(d_frame[CACH + cach_payload_bits[i]]);
			break;
		case 3: // Continue Short_LC
			for (size_t i=0; i<sizeof(cach_payload_bits); i++)
				d_cach_sig.push_back(d_frame[CACH + cach_payload_bits[i]]);
				decode_shortLC();
			break;
	}
	
}

bool
dmr_cai::decode_shortLC()
{
	bool slc[68];

	// deinterleave
	int i, src;
	for (i = 0; i < 67; i++) {
		src = (i * 4) % 67;
		slc[i] = d_cach_sig[src];
	}
	slc[i] = d_cach_sig[i];

	// apply error correction
	CHamming::decode17123(slc + 0);
	CHamming::decode17123(slc + 17);
	CHamming::decode17123(slc + 34);

	// parity check
	for (i = 0; i < 17; i++) {
		if (slc[i+51] != (slc[i+0] ^ slc[i+17] ^ slc[i+34]))
			return false;
	}

	// remove hamming and leave 36 bits of Short LC
	for (i = 17; i < 29; i++) {
		slc[i-5] = slc[i];
	}
	for (i = 34; i < 46; i++) {
		slc[i-10] = slc[i];
	}

	// validate CRC8
	if (crc8((uint8_t*)slc, 36) != 0)
		return false;

	// extract useful data
	if (d_debug >= 10) {
		uint8_t slco, d0, d1, d2;
		slco = 0; d0 = 0; d1 = 0; d2 = 0;
		for (i = 0; i < 4; i++) {
			slco <<= 1;
			slco |= slc[i];
		}
		for (i = 0; i < 8; i++) {
			d0 <<= 1;
			d0 |= slc[i+4];
		}
		for (i = 0; i < 8; i++) {
			d1 <<= 1;
			d1 |= slc[i+12];
		}
		for (i = 0; i < 8; i++) {
			d2 <<= 1;
			d2 |= slc[i+20];
		}
		
		if (d_debug >= 10)
			fprintf(stderr, "SLCO=0x%x, DATA=%02x %02x %02x\n", slco, d0, d1, d2);
	}

	return true;
}
