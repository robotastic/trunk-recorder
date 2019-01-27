// P25 Decoder (C) Copyright 2013, 2014, 2015, 2016, 2017 Max H. Parke KA1RBI
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
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include "rx_sync.h"

#include "bit_utils.h"

#include "check_frame_sync.h"

#include "p25p2_vf.h"
#include "mbelib.h"
#include "ambe.h"
#include "rs.h"
#include "crc16.h"

#include "ysf_const.h"
#include "dmr_const.h"
#include "p25_frame.h"
#include "op25_imbe_frame.h"
#include "software_imbe_decoder.h"
#include "op25_audio.h"

namespace gr{
    namespace op25_repeater{

void rx_sync::cbuf_insert(const uint8_t c) {
	d_cbuf[d_cbuf_idx] = c;
	d_cbuf[d_cbuf_idx + CBUF_SIZE] = c;
	d_cbuf_idx = (d_cbuf_idx + 1) % CBUF_SIZE;
}

void rx_sync::sync_reset(void) {
	d_threshold = 0;
	d_shift_reg = 0;
	d_unmute_until[0] = 0;
	d_unmute_until[1] = 0;
}

static int ysf_decode_fich(const uint8_t src[100], uint8_t dest[32]) {   // input is 100 dibits, result is 32 bits
// return -1 on decode error, else 0
	static const int pc[] = {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1};
	uint8_t buf[100];
	for (int i=0; i<20; i++) {
		for (int j=0; j<5; j++) {
			buf[j+i*5] = src[i+j*20];
		}
	}
	uint8_t dr = 0;
	uint8_t ans[100];
	/* fake trellis decode */
	/* TODO: make less fake */
	for (int i=0; i<100; i++) {
		uint8_t sym = buf[i];
		uint8_t d0  = ((dr << 1) | 0) & 0x1f;
		uint8_t r0 = (pc[ d0 & 0x19 ] << 1) + pc[ d0 & 0x17];
		uint8_t d1  = ((dr << 1) | 1) & 0x1f;
		uint8_t r1 = (pc[ d1 & 0x19 ] << 1) + pc[ d1 & 0x17];
		if (sym == r0) {
			ans[i] = 0;
			dr = d0;
		} else if (sym == r1) {
			ans[i] = 1;
			dr = d1;
		} else {
			return -1;  /* decode error */
		}
	}
	uint8_t fich_bits[12*4];
	store_i(gly24128Dec(load_i(ans+24*0, 24)), fich_bits+12*0, 12);
	store_i(gly24128Dec(load_i(ans+24*1, 24)), fich_bits+12*1, 12);
	store_i(gly24128Dec(load_i(ans+24*2, 24)), fich_bits+12*2, 12);
	store_i(gly24128Dec(load_i(ans+24*3, 24)), fich_bits+12*3, 12);
	uint16_t crc_result = crc16(fich_bits, 48);
	if (crc_result != 0)
		return -1;	// crc failure
	memcpy(dest, fich_bits, 32);
	return 0;
}

void rx_sync::ysf_sync(const uint8_t dibitbuf[], bool& ysf_fullrate, bool& unmute) {
	uint8_t fich_buf[32];
	int rc = ysf_decode_fich(dibitbuf+20, fich_buf);
	if (rc == 0) {
		uint32_t fich = load_i(fich_buf, 32);
		uint32_t dt = (fich >> 8) & 3;
		d_shift_reg = dt;
	}
	switch(d_shift_reg) {
	case 0:		// voice/data mode 1
		unmute = false;
		break;
	case 1:		// data mode
		unmute = false;
		break;
	case 2:		// voice/data mode 2
		unmute = true;
		ysf_fullrate = false;
		break;
	case 3:		// voice fr mode
		unmute = true;
		ysf_fullrate = true;
		break;
	}
	if (d_debug > 5 && !unmute)
		fprintf(stderr, "ysf_sync: muting audio: dt: %d, rc: %d\n", d_shift_reg, rc);
}

void rx_sync::dmr_sync(const uint8_t bitbuf[], int& current_slot, bool& unmute) {
	static const int slot_ids[] = {0, 1, 0, 0, 1, 1, 0, 1};
	int tact;
	int chan;
	int fstype;
	uint8_t tactbuf[sizeof(cach_tact_bits)];

	for (size_t i=0; i<sizeof(cach_tact_bits); i++)
		tactbuf[i] = bitbuf[cach_tact_bits[i]];
	tact = hamming_7_4_decode[load_i(tactbuf, 7)];
	chan = (tact>>2) & 1;
	d_shift_reg = (d_shift_reg << 1) + chan;
	current_slot = slot_ids[d_shift_reg & 7];

	uint64_t sync = load_reg64(bitbuf + (MODE_DATA[RX_TYPE_DMR].sync_offset << 1), MODE_DATA[RX_TYPE_DMR].sync_len);
	if (check_frame_sync(DMR_VOICE_SYNC_MAGIC ^ sync, d_threshold, MODE_DATA[RX_TYPE_DMR].sync_len))
		fstype = 1;
	else if (check_frame_sync(DMR_IDLE_SYNC_MAGIC ^ sync, d_threshold, MODE_DATA[RX_TYPE_DMR].sync_len))
		fstype = 2;
	else
		fstype = 0;
	if (fstype > 0)
		d_expires = d_symbol_count + MODE_DATA[d_current_type].expiration;
	if (fstype == 1) {
		if (!d_unmute_until[current_slot] && d_debug > 5)
			fprintf(stderr, "unmute slot %d\n", current_slot);
		d_unmute_until[current_slot] = d_symbol_count + MODE_DATA[d_current_type].expiration;
	} else if (fstype == 2) {
		if (d_unmute_until[current_slot] && d_debug > 5)
			fprintf(stderr, "mute slot %d\n", current_slot);
		d_unmute_until[current_slot] = 0;
	}
	if (d_unmute_until[current_slot] <= d_symbol_count) {
		d_unmute_until[current_slot] = 0;
	}
	unmute = d_unmute_until[current_slot] > 0;
}

rx_sync::rx_sync(const char * options, int debug) :	// constructor
	d_symbol_count(0),
	d_sync_reg(0),
	d_cbuf_idx(0),
	d_current_type(RX_TYPE_NONE),
	d_rx_count(0),
	d_expires(0),
	d_stereo(false),
	d_debug(debug),
	d_audio(options, debug)
{
	mbe_initMbeParms (&cur_mp[0], &prev_mp[0], &enh_mp[0]);
	mbe_initMbeParms (&cur_mp[1], &prev_mp[1], &enh_mp[1]);
	mbe_initToneParms (&tone_mp[0]);
	mbe_initToneParms (&tone_mp[1]);
	sync_reset();
}

rx_sync::~rx_sync()	// destructor
{
}


void rx_sync::codeword(const uint8_t* cw, const enum codeword_types codeword_type, int slot_id) {
	static const int x=4;
	static const int y=26;
	static const uint8_t majority[8] = {0,0,0,1,0,1,1,1};

	int b[9];
	int U[4];
	uint8_t buf[4*26];
	uint8_t tmp_codeword [144];
	uint32_t E0, ET;
	uint32_t u[8];
	bool do_fullrate = false;
	bool do_silence = false;
	bool do_tone = false;
	voice_codeword fullrate_cw(voice_codeword_sz);

	switch(codeword_type) {
	case CODEWORD_DMR:
		interleaver.process_vcw(cw, b, U);
		if (mbe_dequantizeAmbeTone(&tone_mp[slot_id], U) == 0) {
			do_tone = true;
		} else if (b[0] < 120) { // TODO: handle Erasures/Frame Repeat
			mbe_dequantizeAmbe2250Parms(&cur_mp[slot_id], &prev_mp[slot_id], b);
		}
		break;
	case CODEWORD_DSTAR:
		interleaver.decode_dstar(cw, b, false);
		if (b[0] < 120)
			mbe_dequantizeAmbe2400Parms(&cur_mp[slot_id], &prev_mp[slot_id], b);
		break;
	case CODEWORD_YSF_HALFRATE:	// 104 bits
		for (int i=0; i<x; i++) {
			for (int j=0; j<y; j++) 
				buf[j+i*y] = cw[i+j*x];
		}
		ysf_scramble(buf, 104);
		for (int i=0; i<27; i++)
			tmp_codeword[i] = majority[ (buf[0+i*3] << 2) | (buf[1+i*3] << 1) | buf[2+i*3] ];

		memcpy(tmp_codeword+27, buf+81, 22);
		decode_49bit(b, tmp_codeword);
		if (b[0] < 120)
			mbe_dequantizeAmbe2250Parms(&cur_mp[slot_id], &prev_mp[slot_id], b);
		break;
	case CODEWORD_P25P2:
		break;
	case CODEWORD_P25P1:	// 144 bits
		for (int i=0; i<144; i++)
			fullrate_cw[i] = cw[i];
		imbe_header_decode(fullrate_cw, u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7], E0, ET);
		do_fullrate = true;
		break;
	case CODEWORD_YSF_FULLRATE:	// 144 bits
		for (int i=0; i<144; i++)
			fullrate_cw[i] = cw[ysf_permutation[i]];
		imbe_header_decode(fullrate_cw, u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7], E0, ET);
		do_fullrate = true;
		break;
	}
	if (do_tone) {
		d_software_decoder[slot_id].decode_tone(tone_mp[slot_id].ID, tone_mp[slot_id].AD, &tone_mp[slot_id].n);
	} else {
	mbe_moveMbeParms (&cur_mp[slot_id], &prev_mp[slot_id]);
	if (do_fullrate) {
		d_software_decoder[slot_id].decode(fullrate_cw);
	} else {	/* halfrate */
		if (b[0] >= 120) {
			do_silence = true;
		} else {
			d_software_decoder[slot_id].decode_tap(cur_mp[slot_id].L, 0, cur_mp[slot_id].w0, &cur_mp[slot_id].Vl[1], &cur_mp[slot_id].Ml[1]);
		}
	}
	}
	audio_samples *samples = d_software_decoder[slot_id].audio();
	float snd;
	int16_t samp_buf[NSAMP_OUTPUT];
	for (int i=0; i < NSAMP_OUTPUT; i++) {
		if ((!do_silence) && samples->size() > 0) {
			snd = samples->front();
			samples->pop_front();
		} else {
			snd = 0;
		}
		if (do_fullrate)
			snd *= 32768.0;
		samp_buf[i] = snd;
	}
	output(samp_buf, slot_id);
}

void rx_sync::output(int16_t * samp_buf, const ssize_t slot_id) {
	if (!d_stereo) {
		d_audio.send_audio_channel(samp_buf, NSAMP_OUTPUT * sizeof(int16_t), slot_id);
		return;
	}
}

void rx_sync::rx_sym(const uint8_t sym)
{
	uint8_t bitbuf[864*2];
	enum rx_types sync_detected = RX_TYPE_NONE;
	int current_slot;
	bool unmute;
	uint8_t tmpcw[144];
	bool ysf_fullrate;

	d_symbol_count ++;
	d_sync_reg = (d_sync_reg << 2) | (sym & 3);
	for (int i = 1; i < RX_N_TYPES; i++) {
		if (check_frame_sync(MODE_DATA[i].sync ^ d_sync_reg, (i == d_current_type) ? d_threshold : 0, MODE_DATA[i].sync_len)) {
			sync_detected = (enum rx_types) i;
			break;
		}
	}
	cbuf_insert(sym);
	if (d_current_type == RX_TYPE_NONE && sync_detected == RX_TYPE_NONE)
		return;
	d_rx_count ++;
	if (sync_detected != RX_TYPE_NONE) {
		if (d_current_type != sync_detected) {
			d_current_type = sync_detected;
			d_expires = d_symbol_count + MODE_DATA[d_current_type].expiration;
			d_rx_count = 0;
		}
		if (d_rx_count != MODE_DATA[d_current_type].sync_offset + (MODE_DATA[d_current_type].sync_len >> 1)) {
			if (d_debug)
				fprintf(stderr, "resync at count %d for protocol %s\n", d_rx_count, MODE_DATA[d_current_type].type);
			sync_reset();
			d_rx_count = MODE_DATA[d_current_type].sync_offset + (MODE_DATA[d_current_type].sync_len >> 1);
		} else {
			d_threshold = std::min(d_threshold + 1, 2);
		}
		d_expires = d_symbol_count + MODE_DATA[d_current_type].expiration;
	}
	if (d_symbol_count >= d_expires) {
		if (d_debug)
			fprintf(stderr, "%s: timeout, symbol %d\n", MODE_DATA[d_current_type].type, d_symbol_count);
		d_current_type = RX_TYPE_NONE;
		return;
	}
	if (d_rx_count < MODE_DATA[d_current_type].fragment_len)
		return;
	d_rx_count = 0;
	int start_idx = d_cbuf_idx + CBUF_SIZE - MODE_DATA[d_current_type].fragment_len;
	assert (start_idx >= 0);
	uint8_t * symbol_ptr = d_cbuf+start_idx;
	uint8_t * bit_ptr = symbol_ptr;
	if (d_current_type != RX_TYPE_DSTAR) {
		dibits_to_bits(bitbuf, symbol_ptr, MODE_DATA[d_current_type].fragment_len);
		bit_ptr = bitbuf;
	}
	switch (d_current_type) {
	case RX_TYPE_NONE:
		break;
	case RX_TYPE_P25:
		for (unsigned int codeword_ct=0; codeword_ct < nof_voice_codewords; codeword_ct++) {
			for (unsigned int i=0; i<voice_codeword_sz; i++)
				tmpcw[i] = bit_ptr[voice_codeword_bits[codeword_ct][i]];
			codeword(tmpcw, CODEWORD_P25P1, 0);  // 144 bits
		}
		break;
	case RX_TYPE_DMR:
		dmr_sync(bit_ptr, current_slot, unmute);
		if (!unmute)
			break;
		codeword(symbol_ptr+12, CODEWORD_DMR, current_slot);
		memcpy(tmpcw, symbol_ptr+48, 18);
		memcpy(tmpcw+18, symbol_ptr+90, 18);
		codeword(tmpcw, CODEWORD_DMR, current_slot);
		codeword(symbol_ptr+108, CODEWORD_DMR, current_slot);
		break;
	case RX_TYPE_DSTAR:
		codeword(bit_ptr, CODEWORD_DSTAR, 0);   // 72 bits = 72 symbols
		break;
	case RX_TYPE_YSF:
		ysf_sync(symbol_ptr, ysf_fullrate, unmute);
		if (!unmute)
			break;
		for (int vcw = 0; vcw < 5; vcw++) {
			if (ysf_fullrate) {
				codeword(bit_ptr + 2*(vcw*72 + 120), CODEWORD_YSF_FULLRATE, 0);  // 144 bits
			} else {	/* halfrate */
				codeword(bit_ptr + 2*(vcw*72 + 120 + 20), CODEWORD_YSF_HALFRATE, 0);   // 104 bits
			}
		}
		break;
	case RX_N_TYPES:
		assert(0==1);     /* should not occur */
		break;
	}
}
    } // end namespace op25_repeater
} // end namespace gr
