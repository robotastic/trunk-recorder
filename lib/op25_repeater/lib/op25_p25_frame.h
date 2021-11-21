#ifndef INCLUDED_OP25_P25_FRAME_H
#define INCLUDED_OP25_P25_FRAME_H 1

#include "frame_sync_magics.h"

static const size_t P25_VOICE_FRAME_SIZE = 1728;
static const size_t P25_HEADER_SYMBOLS = 24 + 32 + 1;
static const size_t P25_HEADER_BITS = P25_HEADER_SYMBOLS * 2;

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

#endif   /* INCLUDED_OP25_P25_FRAME_H */
