/* -*- c++ -*- */
/* 
 * DMR Encoder (C) Copyright 2017 Max H. Parke KA1RBI
 * 
 * This file is part of OP25
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "dmr_bs_tx_bb_impl.h"
#include "dmr_const.h"

#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

#if 0
static void print_result(char title[], const uint8_t r[], int len) {
	uint8_t buf[256];
	for (int i=0; i<len; i++){
		buf[i] = r[i] + '0';
	}
	buf[len] = 0;
	printf("%s: %s\n", title, buf);
}
#endif

static uint8_t crc8(const uint8_t bits[], unsigned int len) {
	uint8_t crc=0;
	static const unsigned int K = 8;
	static const uint8_t poly[K+1] = {1,0,0,0,0,0,1,1,1}; // crc8 poly
	uint8_t buf[256];
	if (len+K > sizeof(buf)) {
		fprintf (stderr, "crc8: buffer length %u exceeds maximum %lu\n", len+K, sizeof(buf));
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
	return crc;
}

static bool crc8_ok(const uint8_t bits[], unsigned int len) {
	uint16_t crc = 0;
	for (int i=0; i < 8; i++) {
		crc = (crc << 1) + bits[len+i];
	}
	return (crc == crc8(bits,len));
}

static inline int store_i(int reg, uint8_t val[], int len) {
	for (int i=0; i<len; i++){
		val[i] = (reg >> (len-1-i)) & 1;
	}
}

static void bits_to_dibits(uint8_t* dest, const uint8_t* src, int n_dibits) {
	for (int i=0; i<n_dibits; i++) {
		dest[i] = src[i*2] * 2 + src[i*2+1];
	}
}

static inline int load_i(const uint8_t val[], int len) {
	int acc = 0;
	for (int i=0; i<len; i++){
		acc = (acc << 1) + (val[i] & 1);
	}
	return acc;
}

static void encode_emb(int cc, int pi, int lcss, uint8_t result[16]) {
	int acc = (cc << 3) | (pi << 2) | lcss;
	acc = hamming_16_7[acc];
	for (int i=0; i<16; i++) {
		result[i] = (acc >> (15-i)) & 1;
	}
}

static void encode_embedded(const uint8_t lc[72], uint8_t result[32*4]) {
	uint8_t encode[16*8];

	static const int lengths[] = {11, 11, 10, 10, 10, 10, 10};

	int s_index = 0;

	uint16_t csum = 0;
	for (int i=0; i<9; i++) {
		csum += load_i(&lc[i*8], 8);
	}
	csum = csum % 31;
	for (int i=0; i<7; i++) {
		memcpy(&encode[16*i], &lc[s_index], lengths[i]);
		s_index += lengths[i];
	}
	for (int i=0; i<5; i++) {
		encode[(i+2)*16+10] = (csum >> (4-i)) & 1;
	}
	for (int i=0; i<7; i++) {
		int acc = load_i(&encode[16*i], 11);
		acc = hamming_16_11[acc];
		for (int j=0; j<5; j++){
			encode[i*16+j+11] = (acc >> (4-j)) & 1;
		}
	}
	for (int i=0; i<16; i++) {
		encode[7*16+i] = (encode[0*16+i] + encode[1*16+i] + encode[2*16+i] + \
		                  encode[3*16+i] + encode[4*16+i] + encode[5*16+i] + \
		                  encode[6*16+i]) & 1;
	}
	int resultp = 0;
	for (int i=0; i<16; i++) {
		for (int j=0; j<8; j++){
			result[resultp++] = encode[i+16*j];
		}
	}
}

static void encode_shortlc(const uint8_t lc[28], uint8_t result[17*4]) {
	uint8_t buffer[36];
	uint8_t encode[17*4];

	memcpy(buffer, lc, 28);
	uint8_t crc = crc8(buffer, 28);
	for (int i=0; i<8; i++) {
		buffer[28+i] = (crc >> (7-i)) & 1;
	}
	//print_result("buffer", buffer, 36);

	for (int i=0; i<3; i++) {
		//print_result("buffer-d", &buffer[i*12], 12);
		int acc = load_i(&buffer[i*12], 12);
		acc = hamming_17_12[acc];
		memcpy(&encode[i*17], &buffer[i*12], 12);
		for (int j=0; j<5; j++){
			encode[i*17+j+12] = (acc >> (4-j)) & 1;
		}
	}
	//print_result("encode-0", &encode[0*17], 17);
	//print_result("encode-1", &encode[1*17], 17);
	//print_result("encode-2", &encode[2*17], 17);
	for (int i=0; i<17; i++) {
		encode[3*17+i] = (encode[0*17+i] + encode[1*17+i] + encode[2*17+i]) & 1;
	}
	//print_result("encode-3", &encode[3*17], 17);
	int resultp = 0;
	for (int i=0; i<17; i++) {
		for (int j=0; j<4; j++){
			result[resultp++] = encode[i+17*j];
		}
	}
}

static void generate_cach(uint8_t at, uint8_t tc, uint8_t lcss, const uint8_t cach_bits[17], uint8_t result[24]) {
	int tact = hamming_7_4[ (at << 3) | (tc << 2) | lcss ];
	//printf ("tact %d %x\n", tact, tact);
	//print_result("cach_payload_bits", cach_bits, 17);
	for (int i=0; i<7; i++) {
		result[cach_tact_bits[i]] = (tact >> (6-i)) & 1;
	}
	for (int i=0; i<17; i++) {
		result[cach_payload_bits[i]] = cach_bits[i];
	}
}

namespace gr {
  namespace op25_repeater {

    dmr_bs_tx_bb::sptr
    dmr_bs_tx_bb::make(int verbose_flag, const char * config_file)
    {
      return gnuradio::get_initial_sptr
        (new dmr_bs_tx_bb_impl(verbose_flag, config_file));
    }

//////////////////////////////////////////////////////////////////////////
// accepts ambe codewords (36 dibits, char format) on two input channels
// input channel 0 --> DMR slot 1
// input channel 1 --> DMR slot 2
// outputs dibits (char format) in range 0-3
// symbols are output in bursts of 144 dibits (288 bits) (1 burst per 30 msec.): 
//    CACH (24)
//    voice (108)
//    SYNC or embedded signalling (48)
//    voice (108)
// reads three ambe codewords (216 bits = 108 dibits) for each output burst
// ref. sec. 5.1.5.1
// output dibit map(144 dibits):
//     00 - 11 CACH(12)
//     12 - 47 VC1(36)
//     48 - 65 VC2 part1(18)
//     66 - 89 sync(24)
//     90 -107 VC2 part2(18)
//     108-143 VC3 (36)

static const int MIN_IN = 2;
static const int MAX_IN = 2;

static const int MIN_OUT = 1;
static const int MAX_OUT = 1;

    /*
     * The private constructor
     */
    dmr_bs_tx_bb_impl::dmr_bs_tx_bb_impl(int verbose_flag, const char * config_file)
      : gr::block("dmr_bs_tx_bb",
              gr::io_signature::make (MIN_IN, MAX_IN, 36),
              gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof(char))),
              d_verbose_flag(verbose_flag),
              d_config_file(config_file),
              d_next_burst(0),
              d_next_slot(0)
    {
	d_ts[0] = 0;
	d_ts[1] = 0;
	d_cc[0] = 0;
	d_cc[1] = 0;
	d_so[0] = 0;
	d_so[1] = 0;
	d_ga[0] = 0;
	d_ga[1] = 0;
	d_sa[0] = 0;
	d_sa[1] = 0;
	d_en[0] = 0;
	d_en[1] = 0;
      set_output_multiple(144);
      set_history(3);
      config();
    }

    /*
     * Our virtual destructor.
     */
    dmr_bs_tx_bb_impl::~dmr_bs_tx_bb_impl()
    {
    }

void
dmr_bs_tx_bb_impl::config()
{
	FILE * fp1 = fopen(d_config_file, "r");
	char line[256];
	char * cp;
	if (!fp1) {
		fprintf(stderr, "dmr_bs_tx_bb_impl:config: failed to open %s\n", d_config_file);
		return;
	}
	for (;;) {
		cp = fgets(line, sizeof(line) - 2, fp1);
		if (!cp) break;
		if (line[0] == '#') continue;
		// cach: hashed addr ts1(8) and hashed addr ts2(8)
		// emb: (one per ch) cc(4)
		// group voice channel user PDU: service options (8), group address(24), source address(24)
		// (one per ch)
		if (memcmp(line, "ts1=", 4) == 0)
			sscanf(&line[4], "%d", &d_ts[0]);
		else if (memcmp(line, "ts2=", 4) == 0)
			sscanf(&line[4], "%d", &d_ts[1]);
		else if (memcmp(line, "cc1=", 4) == 0)
			sscanf(&line[4], "%d", &d_cc[0]);
		else if (memcmp(line, "cc2=", 4) == 0)
			sscanf(&line[4], "%d", &d_cc[1]);
		else if (memcmp(line, "so1=", 4) == 0)
			sscanf(&line[4], "%d", &d_so[0]);
		else if (memcmp(line, "so2=", 4) == 0)
			sscanf(&line[4], "%d", &d_so[1]);
		else if (memcmp(line, "ga1=", 4) == 0)
			sscanf(&line[4], "%d", &d_ga[0]);
		else if (memcmp(line, "ga2=", 4) == 0)
			sscanf(&line[4], "%d", &d_ga[1]);
		else if (memcmp(line, "sa1=", 4) == 0)
			sscanf(&line[4], "%d", &d_sa[0]);
		else if (memcmp(line, "sa2=", 4) == 0)
			sscanf(&line[4], "%d", &d_sa[1]);
		else if (memcmp(line, "en1=", 4) == 0)
			sscanf(&line[4], "%d", &d_en[0]);
		else if (memcmp(line, "en2=", 4) == 0)
			sscanf(&line[4], "%d", &d_en[1]);
	}
	fclose(fp1);

	d_ts[0] &= 0xff;
	d_ts[1] &= 0xff;
	d_cc[0] &= 0xf;
	d_cc[1] &= 0xf;
	d_so[0] &= 0xff;
	d_so[1] &= 0xff;
	d_ga[0] &= 0xffffff;
	d_ga[1] &= 0xffffff;
	d_sa[0] &= 0xffffff;
	d_sa[1] &= 0xffffff;
	d_en[0] &= 0x1;
	d_en[1] &= 0x1;
#if 0
	fprintf(stderr, "dmr_bs_tx_bb_impl:config: ts[0] %d ts[1] %d cc[0] %d cc[1] %d\n", d_ts[0], d_ts[1], d_cc[0], d_cc[1]);
	fprintf(stderr, "dmr_bs_tx_bb_impl:config: so[0] %d so[1] %d ga[0] %d ga[1] %d\n", d_so[0], d_so[1], d_ga[0], d_ga[1]);
	fprintf(stderr, "dmr_bs_tx_bb_impl:config: sa[0] %d sa[1] %d\n", d_sa[0], d_sa[1]);
#endif

	// TODO: allow cach and sync data to be reconfigured at runtime
	static uint8_t lc[28]={
		// activity update vol 2 #7.1.3.2
		0,0,0,1,		// opcode
		1,0,0,0,		// group voice on slot 1
		1,0,0,0,		// group voice on slot 2
		0,0,0,0,0,0,0,0,	// ts[0]
		0,0,0,0,0,0,0,0};	// ts[1]

	store_i(d_ts[0], &lc[12], 8);
	store_i(d_ts[1], &lc[20], 8);
	uint8_t shortlc_result[17*4];
	uint8_t cach_result[24];

	encode_shortlc(lc, shortlc_result);

	generate_cach(1, 0, 1, &shortlc_result[0*17], cach_result);
	bits_to_dibits(d_cach[0], cach_result, 12);

	generate_cach(1, 1, 3, &shortlc_result[1*17], cach_result);
	bits_to_dibits(d_cach[1], cach_result, 12);

	generate_cach(1, 0, 3, &shortlc_result[2*17], cach_result);
	bits_to_dibits(d_cach[2], cach_result, 12);

	generate_cach(1, 1, 2, &shortlc_result[3*17], cach_result);
	bits_to_dibits(d_cach[3], cach_result, 12);

	uint8_t full_lc[2][72]; // group voice channel user lc pdu vol 1 #7.1.1.1
	memset(full_lc[0], 0, 72);
	memset(full_lc[1], 0, 72);

	store_i(d_so[0], &full_lc[0][16], 8);
	store_i(d_so[1], &full_lc[1][16], 8);
	store_i(d_ga[0], &full_lc[0][24], 24);
	store_i(d_ga[1], &full_lc[1][24], 24);
	store_i(d_sa[0], &full_lc[0][48], 24);
	store_i(d_sa[1], &full_lc[1][48], 24);

	uint8_t embedded_result[2][32*4];

	encode_embedded(full_lc[0], embedded_result[0]);
	encode_embedded(full_lc[1], embedded_result[1]);

	uint8_t emb_result[2][16*4];
	encode_emb(d_cc[0], 0, 1, &emb_result[0][0*16]);
	encode_emb(d_cc[0], 0, 3, &emb_result[0][1*16]);
	encode_emb(d_cc[0], 0, 3, &emb_result[0][2*16]);
	encode_emb(d_cc[0], 0, 2, &emb_result[0][3*16]);
	encode_emb(d_cc[1], 0, 1, &emb_result[1][0*16]);
	encode_emb(d_cc[1], 0, 3, &emb_result[1][1*16]);
	encode_emb(d_cc[1], 0, 3, &emb_result[1][2*16]);
	encode_emb(d_cc[1], 0, 2, &emb_result[1][3*16]);
	uint8_t emb_buf[48];
	for (int i=0; i<2; i++) {
		for (int j=0; j<4; j++) {
			memcpy(&emb_buf[0], &emb_result[i][j*16], 8);
			memcpy(&emb_buf[8], &embedded_result[i][j*32], 32);
			memcpy(&emb_buf[40], &emb_result[i][j*16+8], 8);
			bits_to_dibits(&d_embedded[i][j*24], emb_buf, 24);
		}
	}
}

void
dmr_bs_tx_bb_impl::forecast(int nof_output_items, gr_vector_int &nof_input_items_reqd)
{
   // reads three ambe codewords (216 bits = 108 dibits) for each 144 dibit output burst
   // the three codewords though are only read from one of the two inputs per burst
   const size_t nof_inputs = nof_input_items_reqd.size();
   const int nof_bursts = nof_output_items / 144.0;
   const int nof_samples_reqd = 3 * ((nof_bursts+1)>>1);
   std::fill(&nof_input_items_reqd[0], &nof_input_items_reqd[nof_inputs], nof_samples_reqd);
}

int 
dmr_bs_tx_bb_impl::general_work (int noutput_items,
			       gr_vector_int &ninput_items,
			       gr_vector_const_void_star &input_items,
			       gr_vector_void_star &output_items)
{

  static const int burst_schedule[2][6] = {
    // -3: idle SYNC
    // -2: voice SYNC
    // -1: RC
    // 0|1|2|3: segment no. of embedded signalling
    {-2, 0, 1, -1, 2, 3},
    {-1, 2, 3, -2, 0, 1}
  };

  int nconsumed[2] = {0,0};
  uint8_t *in[2];
  in[0] = (uint8_t *) input_items[0];
  in[1] = (uint8_t *) input_items[1];
  uint8_t *out = reinterpret_cast<uint8_t*>(output_items[0]);
  int nframes=0;

  for (int n=0;n < (noutput_items/144);n++) {
    // need (at least) three voice codewords
    if ((ninput_items[d_next_slot] - nconsumed[d_next_slot]) < 3) break;

    int cach_id = ((d_next_burst & 1) << 1) + d_next_slot;
    memcpy(&out[0], d_cach[cach_id], 12);
    memcpy(&out[12], &in[d_next_slot][0], 36);
    memcpy(&out[48], &in[d_next_slot][36], 18);
    int schedule = burst_schedule[d_next_slot][d_next_burst];
    if (schedule == -3 || !d_en[d_next_slot])
      memcpy(&out[66], dmr_bs_idle_sync, 24);
    else if (schedule == -2)
      memcpy(&out[66], dmr_bs_voice_sync, 24);
    else if (schedule == -1)
      memset(&out[66], 0, 24);
    else 
      memcpy(&out[66], &d_embedded[d_next_slot][schedule*24], 24);
    memcpy(&out[90], &in[d_next_slot][36+18], 18);
    memcpy(&out[108], &in[d_next_slot][36+36], 36);
    in[d_next_slot] += 108;
    out += 144;
    nconsumed[d_next_slot] += 3;

    d_next_slot = (d_next_slot + 1) & 1;
    if (d_next_slot == 0) {
      d_next_burst = (d_next_burst + 1) % 6;
    }
    nframes++;
  }

  // Tell runtime system how many input items we consumed on
  // each input stream.

  if (nconsumed[0])
    consume (0, nconsumed[0]);
  if (nconsumed[1])
    consume (1, nconsumed[1]);

  // Tell runtime system how many output items we produced.
  return (nframes * 144);
}

  } /* namespace op25_repeater */
} /* namespace gr */
