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

typedef std::vector<uint8_t> packed_codeword;

class p25p2_vf {
public:
	size_t process_vcw(const uint8_t vf[], int* b, int* u = NULL);
	void encode_vcw(uint8_t vf[], const int* b);
	void encode_dstar(uint8_t result[72], const int b[9], bool alt_dstar_interleave);
	size_t decode_dstar(const uint8_t codeword[72], int b[9], bool alt_dstar_interleave);
	void pack_cw(packed_codeword& cw, const int* u);
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
	cw.empty();
	cw.push_back(u[0] >> 4);
	cw.push_back(((u[0] & 0xf) << 4) + (u[1] >> 8));
	cw.push_back(u[1] & 0xff);
	cw.push_back(u[2] >> 3);
	cw.push_back(((u[2] & 0x7) << 5) + (u[3] >> 9));
	cw.push_back((u[3] >> 1) & 0xff);
	cw.push_back((u[3] & 0x1) << 7);
}

#endif /* INCLUDED_P25P2_VF_H */
