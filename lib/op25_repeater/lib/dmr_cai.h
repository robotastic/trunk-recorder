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

#ifndef INCLUDED_DMR_CAI_H
#define INCLUDED_DMR_CAI_H

#include <stdint.h>
#include <vector>

#include "dmr_slot.h"

typedef std::vector<bool> bit_vector;

class dmr_cai {
public:
	dmr_cai(int debug);
	~dmr_cai();
	int load_frame(const uint8_t fr_sym[]);
	inline int chan() { return d_chan; };

private:
	static const int FRAME_SIZE = 288; // frame length in bits
	static const int CACH       =   0; // position of CACH in frame

	uint8_t d_frame[FRAME_SIZE];       // array of bits comprising the current frame
	dmr_slot d_slot[2];
	bit_vector d_cach_sig;
	int d_chan;
	int d_shift_reg;
	int d_debug;

	void extract_cach_fragment();
	bool decode_shortLC();

};

#endif /* INCLUDED_DMR_CAI_H */
