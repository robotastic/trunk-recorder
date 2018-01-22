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

#ifndef INCLUDED_OP25_REPEATER_BIT_UTILS_H
#define INCLUDED_OP25_REPEATER_BIT_UTILS_H

#include <stdint.h>

static inline void store_i(int reg, uint8_t val[], int len) {
	for (int i=0; i<len; i++){
		val[i] = (reg >> (len-1-i)) & 1;
	}
}

static inline int load_i(const uint8_t val[], int len) {
	int acc = 0;
	for (int i=0; i<len; i++){
		acc = (acc << 1) + (val[i] & 1);
	}
	return acc;
}

static inline uint64_t load_reg64(const uint8_t val[], int len) {
	uint64_t acc = 0;
	for (int i=0; i<len; i++){
		acc = (acc << 1) + (val[i] & 1);
	}
	return acc;
}

static inline void dibits_to_bits(uint8_t * bits, const uint8_t * dibits, const int n_dibits) {
	for (int i=0; i<n_dibits; i++) {
		bits[i*2] = (dibits[i] >> 1) & 1;
		bits[i*2+1] = dibits[i] & 1;
	}
}

static inline void bits_to_dibits(uint8_t* dest, const uint8_t* src, int n_dibits) {
	for (int i=0; i<n_dibits; i++) {
		dest[i] = src[i*2] * 2 + src[i*2+1];
	}
}

#endif /* INCLUDED_OP25_REPEATER_BIT_UTILS_H */
