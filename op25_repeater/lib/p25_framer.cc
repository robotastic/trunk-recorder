/* -*- c++ -*- */
/*
 * construct P25 frames out of raw dibits
 * Copyright 2010, 2011, 2012, 2013 KA1RBI
 */

#include <vector>
#include <stdio.h>
#include <stdint.h>
#include <bch.h>
#include <sys/time.h>
#include <op25_p25_frame.h>
#include <p25_framer.h>

static const int max_frame_lengths[16] = {
// lengths are in bits, not symbols
	792,	// 0 - pdu
	0, 0,	// 1, 2 - undef
	144,	// 3 - tdu
	0, 	// 4 - undef
	P25_VOICE_FRAME_SIZE,	// 5 - ldu1
	0,	// 6 - undef
	720,	// 7 - trunking (FIXME: are there systems with longer ones?)
	0,	// 8 - undef
	0,	// 9 - VSELP "voice PDU"
	P25_VOICE_FRAME_SIZE,	// a - ldu2
	0,	// b - undef
	720,	// c - VSELP "voice PDU"
	0, 0,	// d, e - undef
	432	// f - tdu
};

// constructor
p25_framer::p25_framer() :
	reverse_p(0),
	nid_syms(0),
	next_bit(0),
	nid_accum(0),
	frame_size_limit(0),
	symbols_received(0),
	frame_body(P25_VOICE_FRAME_SIZE)
{
}

// destructor
p25_framer::~p25_framer ()
{
}

// count no. of 1 bits in masked, xor'ed, FS, return true if < threshold
static inline bool check_frame_sync(uint64_t ch, int err_threshold) {
	int errs=0;
	for (int i=0; i < 48; i++) {
		errs += (ch & 1);
		ch = ch >> 1;
	}
	if (errs <= err_threshold) return 1;
	return 0;
}

/*
 * Process the 64 bits after the frame sync, which should be the frame NID
 * 1. verify bch and reject if bch cannot be decoded
 * 2. extract NAC and DUID
 * Returns false if decode failure, else true
 */
bool p25_framer::nid_codeword(uint64_t acc) {
	bit_vector cw(64);
	bool low = acc & 1;
	// for bch, split bits into codeword vector
	for (int i=0; i < 64; i++) {
		acc >>= 1;
		cw[i] = acc & 1;
	}

	// do bch decode
	int rc = bchDec(cw);

	// check if bch decode unsuccessful
	if (rc < 0) {
		return false;
	}

	bch_errors = rc;

	// load corrected bch bits into acc
	acc = 0;
	for (int i=63; i>=0; i--) {
		acc |= cw[i];
		acc <<= 1;
	}
	acc |= low;   // FIXME

	nid_word = acc;		// reconstructed NID
	// extract nac and duid
	nac  = (acc >> 52) & 0xfff;
	duid = (acc >> 48) & 0x00f;

	return true;
}

/*
 * rx_sym: called once per received symbol
 * 1. looks for flags sequences
 * 2. after flags detected (nid_syms > 0), accumulate 64-bit NID word
 * 3. do BCH check on completed NID
 * 4. after valid BCH check (next_bit > 0), accumulate frame data bits
 *
 * Returns true when complete frame received, else false
 */
bool p25_framer::rx_sym(uint8_t dibit) {
	symbols_received++;
        bool rc = false;
	dibit ^= reverse_p;
	// FIXME assert(dibit >= 0 && dibit <= 3)
	nid_accum <<= 2;
	nid_accum |= dibit;
	if (nid_syms == 12) {
		// ignore status dibit
		nid_accum >>= 2;
	} else if (nid_syms >= 33) {
		// nid completely received
		nid_syms = 0;
		bool bch_rc = nid_codeword(nid_accum);
		if (bch_rc) {   // if ok to start accumulating frame data
			next_bit = 48 + 64;
			frame_size_limit = max_frame_lengths[duid];
			if (frame_size_limit <= next_bit)
				// size isn't known a priori -
				// fall back to max. size and wait for next FS
				frame_size_limit = P25_VOICE_FRAME_SIZE;
		}
	}
	if (nid_syms > 0) // if nid accumulation in progress
		nid_syms++; // count symbols in nid

	if(check_frame_sync((nid_accum & P25_FRAME_SYNC_MASK) ^ P25_FRAME_SYNC_MAGIC, 6)) {
		nid_syms = 1;
	}
	if(check_frame_sync((nid_accum & P25_FRAME_SYNC_MASK) ^ P25_FRAME_SYNC_REV_P, 0)) {
		nid_syms = 1;
		reverse_p ^= 0x02;   // auto flip polarity reversal
		fprintf(stderr, "Reversed FS polarity detected - autocorrecting\n");
	}
	if(check_frame_sync((nid_accum & P25_FRAME_SYNC_MASK) ^ 0x001050551155LL, 0)) {
		fprintf(stderr, "tuning error -1200\n");
	}
	if(check_frame_sync((nid_accum & P25_FRAME_SYNC_MASK) ^ 0xFFEFAFAAEEAALL, 0)) {
		fprintf(stderr, "tuning error +1200\n");
	}
	if(check_frame_sync((nid_accum & P25_FRAME_SYNC_MASK) ^ 0xAA8A0A008800LL, 0)) {
		fprintf(stderr, "tuning error +/- 2400\n");
	}
	if (next_bit > 0) {
		frame_body[next_bit++] = (dibit >> 1) & 1;
		frame_body[next_bit++] =  dibit       & 1;
	}
	// dispose of received frame (if exists) and:
	// 1. complete frame is received, or
	// 2. flags is received
	if ((next_bit > 0) && (next_bit >= frame_size_limit || nid_syms > 0)) {
		if (nid_syms > 0)  // if this was triggered by FS
			next_bit -= 48;	// FS has been added to body - remove it
		p25_setup_frame_header(frame_body, nid_word);
		frame_size = next_bit;
		next_bit = 0;
		rc = true;	// set rc indicating frame available
	}
	return rc;
}
