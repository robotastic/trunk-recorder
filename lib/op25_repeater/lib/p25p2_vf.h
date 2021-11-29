// P25 TDMA Decoder (C) Copyright 2013, 2014 Max H. Parke KA1RBI
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

#ifndef INCLUDED_P25P2_VF_H
#define INCLUDED_P25P2_VF_H

#include <stdint.h>
#include <vector>
#include "mbelib.h"

typedef std::vector<uint8_t> packed_codeword;

class p25p2_vf {
public:
	size_t process_vcw(mbe_errs* errs_mp, const uint8_t vf[], int* b, int* U = NULL);
	void encode_vcw(uint8_t vf[], const int* b);
	void encode_dstar(uint8_t result[72], const int b[9], bool alt_dstar_interleave);
	size_t decode_dstar(const uint8_t codeword[72], int b[9], bool alt_dstar_interleave);
	void pack_cw(packed_codeword& cw, const int* u);
	void unpack_cw(const packed_codeword& cw, int* u);
	void unpack_b(int* b, const int* u);
private:
	void extract_vcw(const uint8_t _vf[], int& _c0, int& _c1, int& _c2, int& _c3);
	void interleave_vcw(uint8_t _vf[], int _c0, int _c1, int _c2, int _c3);
};

/*
 * Pack 49 bit AMBE parameters into uint8_t vector
 */
inline
void p25p2_vf::pack_cw(packed_codeword& cw, const int* u)
{
	cw.clear();
	cw.push_back(u[0] >> 4);
	cw.push_back(((u[0] & 0xf) << 4) + (u[1] >> 8));
	cw.push_back(u[1] & 0xff);
	cw.push_back(u[2] >> 3);
	cw.push_back(((u[2] & 0x7) << 5) + (u[3] >> 9));
	cw.push_back((u[3] >> 1) & 0xff);
	cw.push_back((u[3] & 0x1) << 7);
}

/*
 * Unpack 49 bit AMBE parameters from uint8_t vector
 */
inline
void p25p2_vf::unpack_cw(const packed_codeword& cw, int* u)
{
	u[0] = ((cw[0] & 0xff) << 4) + ((cw[1] & 0xf0) >> 4);
	u[1] = ((cw[1] & 0x0f) << 8) + (cw[2] & 0xff);
	u[2] = ((cw[3] & 0xff) << 3) + ((cw[4] & 0xe0) >> 5);
	u[3] = ((cw[4] & 0x1f) << 9) + ((cw[5] & 0xff) << 1) + ((cw[6] & 0x80) >> 7);
}

/*
 * Unpack b[0-8] codec params from u[0-3] codewords
 */
inline
void p25p2_vf::unpack_b(int* b, const int* u)
{
	b[0] = ((u[0] >> 5) & 0x78) + ((u[3] >> 9) & 0x7);
	b[1] = ((u[0] >> 3) & 0x1e) + ((u[3] >> 13) & 0x1);
	b[2] = ((u[0] << 1) & 0x1e) + ((u[3] >> 12) & 0x1);
	b[3] = ((u[1] >> 3) & 0x1fe) + ((u[3] >> 8) & 0x1);
	b[4] = ((u[1] << 3) & 0x78) + ((u[3] >> 5) & 0x7);
	b[5] = ((u[2] >> 6) & 0x1e) + ((u[3] >> 4) & 0x1);
	b[6] = ((u[2] >> 3) & 0x0e) + ((u[3] >> 3) & 0x1);
	b[7] = ( u[2]       & 0x0e) + ((u[3] >> 2) & 0x1);
	b[8] = ((u[2] << 2) & 0x04) + ( u[3]       & 0x3);
}

#endif /* INCLUDED_P25P2_VF_H */
