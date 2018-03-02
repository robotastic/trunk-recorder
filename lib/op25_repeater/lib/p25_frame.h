/* 
 * 
 * Copyright 2009, 2010, 2011, 2012, 2013 KA1RBI
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

#ifndef INCLUDED_P25_FRAME_H
#define INCLUDED_P25_FRAME_H 1

#include <vector>
typedef std::vector<bool> bit_vector;
namespace gr {
  namespace op25_repeater {
static const size_t P25_VOICE_FRAME_SIZE = 1728;
static const size_t P25_HEADER_SYMBOLS = 24 + 32 + 1;
static const size_t P25_HEADER_BITS = P25_HEADER_SYMBOLS * 2;

static const uint64_t P25_FRAME_SYNC_MAGIC = 0x5575F5FF77FFLL;
static const uint64_t P25_FRAME_SYNC_REV_P = 0x5575F5FF77FFLL ^ 0xAAAAAAAAAAAALL;
static const uint64_t P25_FRAME_SYNC_MASK  = 0xFFFFFFFFFFFFLL;

/* Given a 64-bit frame header word and a frame body which is to be initialized
 * 1. Place flags at beginning of frame body
 * 2. Store 64-bit frame header word
 * 3. FIXME Place first status symbol
 */
static inline void
p25_setup_frame_header(bit_vector& frame_body, uint64_t hw) {
	uint64_t acc = P25_FRAME_SYNC_MAGIC;
	for (int i=47; i>=0; i--) {
		frame_body[ i ] = acc & 1;
		acc >>= 1;
	}
	acc = hw;
	for (int i=113; i>=72; i--) {
		frame_body[ i ] = acc & 1;
		acc >>= 1;
	}
	// FIXME: insert proper status dibit bits at 70, 71
	frame_body[70] = 1;
	frame_body[71] = 0;
	for (int i=69; i>=48; i--) {
		frame_body[ i ] = acc & 1;
		acc >>= 1;
	}
}

  } // namespace op25_repeater
}  // namespace gr

#endif   /* INCLUDED_P25_FRAME_H */
