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

#include <stdint.h>
#include <map>
#include <string.h>
#include <string>
#include <iostream>
#include <assert.h>

#include "p25p2_duid.h"
#include "p25p2_sync.h"
#include "p25p2_tdma.h"
#include "p25p2_vf.h"
#include "mbelib.h"
#include "ambe.h"

static const int BURST_SIZE = 180;
static const int SUPERFRAME_SIZE = (12*BURST_SIZE);

static uint16_t crc12(const uint8_t bits[], unsigned int len) {
	uint16_t crc=0;
	static const unsigned int K = 12;
	static const uint8_t poly[K+1] = {1,1,0,0,0,1,0,0,1,0,1,1,1}; // p25 p2 crc 12 poly
	uint8_t buf[256];
	if (len+K > sizeof(buf)) {
		fprintf (stderr, "crc12: buffer length %u exceeds maximum %lu\n", len+K, sizeof(buf));
		return 0;
	}
	memset (buf, 0, sizeof(buf));
	for (int i=0; i<len; i++){
		buf[i] = bits[i];
	}
	for (int i=0; i<len; i++)
		if (buf[i])
			for (int j=0; j<K+1; j++)
				buf[i+j] ^= poly[j];
	for (int i=0; i<K; i++){
		crc = (crc << 1) + buf[len + i];
	}
	return crc ^ 0xfff;
}

static bool crc12_ok(const uint8_t bits[], unsigned int len) {
	uint16_t crc = 0;
	for (int i=0; i < 12; i++) {
		crc = (crc << 1) + bits[len+i];
	}
	return (crc == crc12(bits,len));
}

p25p2_tdma::p25p2_tdma(int slotid, int debug, std::deque<int16_t> &qptr) :	// constructor
	tdma_xormask(new uint8_t[SUPERFRAME_SIZE]),
	symbols_received(0),
	packets(0),
	d_slotid(slotid),
	output_queue_decode(qptr),
	d_debug(debug),
	crc_errors(0),
	p2framer()
{
	assert (slotid == 0 || slotid == 1);
	mbe_initMbeParms (&cur_mp, &prev_mp, &enh_mp);
}

bool p25p2_tdma::rx_sym(uint8_t sym)
{
	symbols_received++;
	return p2framer.rx_sym(sym);
}

void p25p2_tdma::set_slotid(int slotid)
{
	assert (slotid == 0 || slotid == 1);
	d_slotid = slotid;
}

p25p2_tdma::~p25p2_tdma()	// destructor
{
	delete[](tdma_xormask);
}

void
p25p2_tdma::set_xormask(const char*p) {
	for (int i=0; i<SUPERFRAME_SIZE; i++)
		tdma_xormask[i] = p[i] & 3;
}

int p25p2_tdma::process_mac_pdu(const uint8_t byte_buf[], unsigned int len) 
{
	unsigned int opcode = (byte_buf[0] >> 5) & 0x7;
	unsigned int offset = (byte_buf[0] >> 2) & 0x7;
	// maps sacch opcodes into phase I duid values 
	static const int opcode_map[8] = {3, 5, 3, 3, 5, 3, 3, 3};
	return opcode_map[opcode];
	// TODO: decode MAC PDU's
}

int p25p2_tdma::handle_acch_frame(const uint8_t dibits[], bool fast) 
{
	int rc = -1;
	uint8_t bits[512];
	uint8_t byte_buf[32];
	unsigned int bufl=0;
	unsigned int len=0;
	if (fast) {
		for (int i=11; i < 11+36; i++) {
			bits[bufl++] = (dibits[i] >> 1) & 1;
			bits[bufl++] = dibits[i] & 1;
		}
		for (int i=48; i < 48+31; i++) {
			bits[bufl++] = (dibits[i] >> 1) & 1;
			bits[bufl++] = dibits[i] & 1;
		}
		for (int i=100; i < 100+32; i++) {
			bits[bufl++] = (dibits[i] >> 1) & 1;
			bits[bufl++] = dibits[i] & 1;
		}
		for (int i=133; i < 133+36; i++) {
			bits[bufl++] = (dibits[i] >> 1) & 1;
			bits[bufl++] = dibits[i] & 1;
		}
	} else {
		for (int i=11; i < 11+36; i++) {
			bits[bufl++] = (dibits[i] >> 1) & 1;
			bits[bufl++] = dibits[i] & 1;
		}
		for (int i=48; i < 48+84; i++) {
			bits[bufl++] = (dibits[i] >> 1) & 1;
			bits[bufl++] = dibits[i] & 1;
		}
		for (int i=133; i < 133+36; i++) {
			bits[bufl++] = (dibits[i] >> 1) & 1;
			bits[bufl++] = dibits[i] & 1;
		}
	}
	// FIXME: TODO: add RS decode
	if (fast)
		len = 144;
	else
		len = 168;
	if (crc12_ok(bits, len)) {
		for (int i=0; i<len/8; i++) {
			byte_buf[i] = (bits[i*8 + 0] << 7) + (bits[i*8 + 1] << 6) + (bits[i*8 + 2] << 5) + (bits[i*8 + 3] << 4) + (bits[i*8 + 4] << 3) + (bits[i*8 + 5] << 2) + (bits[i*8 + 6] << 1) + (bits[i*8 + 7] << 0);
		}
		rc = process_mac_pdu(byte_buf, len/8);
	} else {
		crc_errors++;
	}
	return rc;
}

void p25p2_tdma::handle_voice_frame(const uint8_t dibits[]) 
{
	static const int NSAMP_OUTPUT=160;
	int b[9];
	int16_t snd;
	int K;
	int rc = -1;

	vf.process_vcw(dibits, b);
	if (b[0] < 120)
		rc = mbe_dequantizeAmbe2250Parms (&cur_mp, &prev_mp, b);
	/* FIXME: check RC */
	K = 12;
	if (cur_mp.L <= 36)
		K = int(float(cur_mp.L + 2.0) / 3.0);
	if (rc == 0)
		software_decoder.decode_tap(cur_mp.L, K, cur_mp.w0, &cur_mp.Vl[1], &cur_mp.Ml[1]);
	audio_samples *samples = software_decoder.audio();
	for (int i=0; i < NSAMP_OUTPUT; i++) {
		if (samples->size() > 0) {
			snd = (int16_t)(samples->front());
			samples->pop_front();
		} else {
			snd = 0;
		}
		output_queue_decode.push_back(snd);
	}
	mbe_moveMbeParms (&cur_mp, &prev_mp);
	mbe_moveMbeParms (&cur_mp, &enh_mp);
}

int p25p2_tdma::handle_frame(void)
{
	uint8_t dibits[180];
	int rc;
	for (int i=0; i<sizeof(dibits); i++)
		dibits[i] = p2framer.d_frame_body[i*2+1] + (p2framer.d_frame_body[i*2] << 1);
	rc = handle_packet(dibits);
	return rc;
}

/* returns true if in sync and slot matches current active slot d_slotid */
int p25p2_tdma::handle_packet(const uint8_t dibits[]) 
{
	int rc = -1;
	static const int which_slot[] = {0,1,0,1,0,1,0,1,0,1,1,0};
	packets++;
	sync.check_confidence(dibits);
	if (!sync.in_sync())
		return -1;
	const uint8_t* burstp = &dibits[10];
	uint8_t xored_burst[BURST_SIZE - 10];
	int burst_type = duid.duid_lookup(duid.extract_duid(burstp));
	if (which_slot[sync.tdma_slotid()] != d_slotid) // active slot?
		return -1;
	for (int i=0; i<BURST_SIZE - 10; i++) {
		xored_burst[i] = burstp[i] ^ tdma_xormask[sync.tdma_slotid() * BURST_SIZE + i];
	}
	if (d_debug) {
		fprintf(stderr, "p25p2_tdma: burst type %d symbols %u packets %u\n", burst_type, symbols_received, packets);
	}
	if (burst_type == 0 || burst_type == 6)	{ // 4v or 2v (voice) ?
		handle_voice_frame(&xored_burst[11]);
		handle_voice_frame(&xored_burst[48]);
		if (burst_type == 0) { // 4v ?
			handle_voice_frame(&xored_burst[96]);
			handle_voice_frame(&xored_burst[133]);
		}
		return -1;
	} else if (burst_type == 3) { // scrambled sacch
		rc = handle_acch_frame(xored_burst, 0);
	} else if (burst_type == 9) { // scrambled facch
		rc = handle_acch_frame(xored_burst, 1);
	} else if (burst_type == 12) { // unscrambled sacch
		rc = handle_acch_frame(burstp, 0);
	} else if (burst_type == 15) { // unscrambled facch
		rc = handle_acch_frame(burstp, 1);
	} else {
		// unsupported type duid
		return -1;
	}
	return rc;
}
