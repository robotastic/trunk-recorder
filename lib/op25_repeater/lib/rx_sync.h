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

#ifndef INCLUDED_RX_SYNC_H
#define INCLUDED_RX_SYNC_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <deque>
#include <assert.h>

#include "bit_utils.h"
#include "check_frame_sync.h"

#include "p25p2_vf.h"
#include "mbelib.h"
#include "ambe.h"

#include "ysf_const.h"
#include "dmr_const.h"
#include "dmr_cai.h"
#include "p25_frame.h"
#include "op25_imbe_frame.h"
#include "software_imbe_decoder.h"
#include "op25_audio.h"
#include "log_ts.h"

namespace gr{
    namespace op25_repeater{

static const uint64_t DSTAR_FRAME_SYNC_MAGIC = 0x444445101440LL;  // expanded into dibits

enum rx_types {
	RX_TYPE_NONE=0,
	RX_TYPE_P25,
	RX_TYPE_DMR,
	RX_TYPE_DSTAR,
	RX_TYPE_YSF,
	RX_N_TYPES
};   // also used as array index

static const struct _mode_data {
	const char * type;
	int sync_len;
	int sync_offset;
	int fragment_len;   // symbols
	int expiration;
	uint64_t sync;
} MODE_DATA[RX_N_TYPES] = {
	{"NONE",   0,0,0,0},
	{"P25",    48,0,864,1728},
	{"DMR",    48,66,144,1728},
	{"DSTAR",  48,72,96,2016*2},
	{"YSF",    40,0,480,480*2}
};   // index order must match rx_types enum

static const int KNOWN_MAGICS = 5;
static const struct _sync_magic {
	int type;
	uint64_t magic;
} SYNC_MAGIC[KNOWN_MAGICS] = {
	{RX_TYPE_P25, P25_FRAME_SYNC_MAGIC},
	{RX_TYPE_DMR, DMR_VOICE_SYNC_MAGIC},
	{RX_TYPE_DMR, DMR_DATA_SYNC_MAGIC},
	{RX_TYPE_DSTAR, DSTAR_FRAME_SYNC_MAGIC},
	{RX_TYPE_YSF, YSF_FRAME_SYNC_MAGIC}
}; // maps sync patterns to protocols

enum codeword_types {
	CODEWORD_P25P1,
	CODEWORD_P25P2,
	CODEWORD_DMR,
	CODEWORD_DSTAR,
	CODEWORD_YSF_FULLRATE,
	CODEWORD_YSF_HALFRATE
};

class rx_sync {
public:
	void rx_sym(const uint8_t sym);
	void sync_reset(void);
	rx_sync(const char * options, int debug);
	~rx_sync();
private:
	void cbuf_insert(const uint8_t c);
	void dmr_sync(const uint8_t bitbuf[], int& current_slot, bool& unmute);
	void ysf_sync(const uint8_t dibitbuf[], bool& ysf_fullrate, bool& unmute);
	void codeword(const uint8_t* cw, const enum codeword_types codeword_type, int slot_id);
	void output(int16_t * samp_buf, const ssize_t slot_id);
	static const int CBUF_SIZE=864;
	static const int NSAMP_OUTPUT = 160;

	unsigned int d_symbol_count;
	uint64_t d_sync_reg;
	uint8_t d_cbuf[CBUF_SIZE*2];
	unsigned int d_cbuf_idx;
	enum rx_types d_current_type;
	int d_threshold;
	int d_rx_count;
	unsigned int d_expires;
	int d_shift_reg;
	unsigned int d_unmute_until[2];
	p25p2_vf interleaver;
	mbe_parms cur_mp[2];
	mbe_parms prev_mp[2];
	mbe_parms enh_mp[2];
	mbe_tone tone_mp[2];
	software_imbe_decoder d_software_decoder[2];
	std::deque<int16_t> d_output_queue[2];
	dmr_cai dmr;
	bool d_stereo;
	int d_debug;
	op25_audio d_audio;
	log_ts logts;
};

    } // end namespace op25_repeater
} // end namespace gr
#endif // INCLUDED_RX_SYNC_H
