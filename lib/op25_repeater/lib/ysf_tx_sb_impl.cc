/* -*- c++ -*- */
/* 
 * YSF Encoder (C) Copyright 2017 Max H. Parke KA1RBI
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
#include "mbelib.h"
#include "p25p2_vf.h"
#include "ysf_tx_sb_impl.h"
#include "ysf_const.h"
#include <op25_imbe_frame.h>

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
static inline void print_result(char title[], const uint8_t r[], int len) {
	uint8_t buf[256];
	for (int i=0; i<len; i++){
		buf[i] = r[i] + '0';
	}
	buf[len] = 0;
	printf("%s: %s\n", title, buf);
}
#endif

static inline int store_i(int reg, uint8_t val[], int len) {
	for (int i=0; i<len; i++){
		val[i] = (reg >> (len-1-i)) & 1;
	}
}

static inline void bits_to_dibits(uint8_t* dest, const uint8_t* src, int n_dibits) {
	for (int i=0; i<n_dibits; i++) {
		dest[i] = src[i*2] * 2 + src[i*2+1];
	}
}

static inline void bool_to_dibits(uint8_t* dest, const std::vector<bool> src, int n_dibits) {
	for (int i=0; i<n_dibits; i++) {
		int l = src[i*2] ? 1 : 0;
		int r = src[i*2+1] ? 1 : 0;
		dest[i] = l * 2 + r;
	}
}

static inline int load_i(const uint8_t val[], int len) {
	int acc = 0;
	for (int i=0; i<len; i++){
		acc = (acc << 1) + (val[i] & 1);
	}
	return acc;
}

// unpacks bytes into bits, len is length of result
static inline void unpack_bytes(uint8_t result[], const char src[], int len) {
	static const int nbytes = len / 8;
	int outp = 0;
	for (int i=0; i < len; i++) {
		result[i] = (src[i>>3] >> (7-(i%8))) & 1;
	}
}

static inline uint16_t crc16(const uint8_t buf[], int len) {
        uint32_t poly = (1<<12) + (1<<5) + (1<<0);
        uint32_t crc = 0;
        for(int i=0; i<len; i++) {
                uint8_t bit = buf[i] & 1;
                crc = ((crc << 1) | bit) & 0x1ffff;
                if (crc & 0x10000)
                        crc = (crc & 0xffff) ^ poly;
	}
        crc = crc ^ 0xffff;
        return crc & 0xffff;
}

// trellis_1_2 encode: source is in bits, result in dibits
static inline void trellis_encode(uint8_t result[], const uint8_t source[], int result_len)
{
	static const int pc[] = {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1};
	int reg = 0;
	for (int i=0; i<result_len; i++) {
		reg = (reg << 1) | source[i];
		result[i] = (pc[reg & 0x19]<<1) + pc[reg & 0x17];
	}
}

static inline void trellis_interleave(uint8_t result[], const uint8_t input[], int x, int y)
{
	static uint8_t tmp_result[200];
	assert (x*y <= sizeof(tmp_result));

	trellis_encode(tmp_result, input, x*y);

	for (int i=0; i<x; i++) {
		for (int j=0; j<y; j++) {
			result[i+j*x] = tmp_result[j+i*y];
		}
	}
}

static inline void generate_fich(uint8_t result[100], int fi, int cs, int cm, int bn, int bt, int fn, int ft, int rsv, int dev, int mr, int voip, int dt, int sql, int sc)
{
	
	int reg;
	reg =               fi & 3;
	reg = (reg << 2) + (cs & 3);
	reg = (reg << 2) + (cm & 3);
	reg = (reg << 2) + (bn & 3);
	reg = (reg << 2) + (bt & 3);
	reg = (reg << 3) + (fn & 7);
	reg = (reg << 3) + (ft & 7);
	reg = (reg << 1) + (rsv & 1);
	reg = (reg << 1) + (dev & 1);
	reg = (reg << 3) + (mr & 7);
	reg = (reg << 1) + (voip & 1);
	reg = (reg << 2) + (dt & 3);
	reg = (reg << 1) + (sql & 1);
	reg = (reg << 7) + (sc & 0x7f);

	uint8_t fich_bits[48];
	memset(fich_bits, 0, sizeof(fich_bits));
	store_i(reg, fich_bits, 32);
	uint16_t crc = crc16(fich_bits, 48);
	store_i(crc, fich_bits+32, 16);

	uint8_t pre_trellis[100];
	store_i(gly_24_12[ load_i(fich_bits+12*0, 12) ], pre_trellis+24*0, 24);
	store_i(gly_24_12[ load_i(fich_bits+12*1, 12) ], pre_trellis+24*1, 24);
	store_i(gly_24_12[ load_i(fich_bits+12*2, 12) ], pre_trellis+24*2, 24);
	store_i(gly_24_12[ load_i(fich_bits+12*3, 12) ], pre_trellis+24*3, 24);
	pre_trellis[96] = 0;
	pre_trellis[97] = 0;
	pre_trellis[98] = 0;
	pre_trellis[99] = 0;

	trellis_interleave(result, pre_trellis, 20, 5);
}

// encode DCH - input is bits, result is dibits
static inline void generate_dch(uint8_t result[180], const uint8_t input[160])
{
	uint8_t pre_trellis[180];
	memset(pre_trellis, 0, sizeof(pre_trellis));

	memcpy(pre_trellis, input, 160);
	ysf_scramble(pre_trellis, 160);

	uint16_t crc = crc16(pre_trellis, 176);
	store_i(crc, pre_trellis+160, 16);
	pre_trellis[176] = 0;
	pre_trellis[177] = 0;
	pre_trellis[178] = 0;
	pre_trellis[179] = 0;

	trellis_interleave(result, pre_trellis, 20, 9);
}

// encode DCH V/D mode type 2 - input is bits, result is dibits
static inline void generate_dch_vd2(uint8_t result[100], const uint8_t input[80])
{
	uint8_t pre_trellis[100];
	memset(pre_trellis, 0, sizeof(pre_trellis));

	memcpy(pre_trellis, input, 80);
	ysf_scramble(pre_trellis, 80);

	uint16_t crc = crc16(pre_trellis, 96);
	store_i(crc, pre_trellis+80, 16);
	pre_trellis[96] = 0;
	pre_trellis[97] = 0;
	pre_trellis[98] = 0;
	pre_trellis[99] = 0;

	trellis_interleave(result, pre_trellis, 20, 5);
}

// encode VCH V/D mode type 2 - input is bits, result is dibits
static inline void generate_vch_vd2(uint8_t result[52], const uint8_t input[49])
{
	uint8_t buf[104];

	for (int i=0; i<27; i++) {
		buf[0+i*3] = input[i];
		buf[1+i*3] = input[i];
		buf[2+i*3] = input[i];
	}
	memcpy(buf+81, input+27, 22);
	buf[103] = 0;
	ysf_scramble(buf, 104);

	uint8_t bit_result[104];
	int x=4;
	int y=26;
	for (int i=0; i<x; i++) {
		for (int j=0; j<y; j++) {
			bit_result[i+j*x] = buf[j+i*y];
		}
	}
	bits_to_dibits(result, bit_result, 52);
}

namespace gr {
  namespace op25_repeater {

    ysf_tx_sb::sptr
    ysf_tx_sb::make(int verbose_flag, const char * config_file, bool fullrate_mode)
    {
      return gnuradio::get_initial_sptr
        (new ysf_tx_sb_impl(verbose_flag, config_file, fullrate_mode));
    }

//////////////////////////////////////////////////////////////////////////

static const int MIN_IN = 1;
static const int MAX_IN = 1;

static const int MIN_OUT = 1;
static const int MAX_OUT = 1;

    /*
     * The private constructor
     */
    ysf_tx_sb_impl::ysf_tx_sb_impl(int verbose_flag, const char * config_file, bool fullrate_mode)
      : gr::block("ysf_tx_sb",
              gr::io_signature::make (MIN_IN, MAX_IN, sizeof(short)),
              gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof(char))),
              d_verbose_flag(verbose_flag),
              d_config_file(config_file),
              d_fullrate_mode(fullrate_mode),
              d_ft(0),
              d_mr(0),
              d_sq(0),
              d_sc(0),
              d_dev(0),
              d_voip(0)
    {
      memset(d_dest, ' ', 10);
      memset(d_src, ' ', 10);
      memset(d_down, ' ', 10);
      memset(d_up, ' ', 10);
      memset(d_rem12, ' ', 10);
      memset(d_rem34, ' ', 10);
      set_output_multiple(480);
      d_halfrate_encoder.set_49bit_mode();
      config();
    }

    /*
     * Our virtual destructor.
     */
    ysf_tx_sb_impl::~ysf_tx_sb_impl()
    {
    }

void ysf_tx_sb_impl::write_fich(uint8_t result[100]) {

}

static inline void sstring(const char s[], char dest[10]) {
	memset(dest, ' ', 10);
	memcpy(dest, s, std::min(strlen(s), (size_t)10));
        for (int i=0; i<10; i++) {
		if (dest[i] < ' ')
			dest [i] = ' ';
	}
}

void
ysf_tx_sb_impl::config()
{
	FILE * fp1 = fopen(d_config_file, "r");
	char line[256];
	char * cp;
	if (!fp1) {
		fprintf(stderr, "ysf_tx_sb_impl:config: failed to open %s\n", d_config_file);
		return;
	}
	for (;;) {
		cp = fgets(line, sizeof(line) - 2, fp1);
		if (!cp) break;
		if (line[0] == '#') continue;
		if (memcmp(line, "ft=", 3) == 0)
			sscanf(&line[3], "%d", &d_ft);
		else if (memcmp(line, "mr=", 3) == 0)
			sscanf(&line[3], "%d", &d_mr);
		else if (memcmp(line, "sq=", 3) == 0)
			sscanf(&line[3], "%d", &d_sq);
		else if (memcmp(line, "sc=", 3) == 0)
			sscanf(&line[3], "%d", &d_sc);
		else if (memcmp(line, "dev=", 4) == 0)
			sscanf(&line[4], "%d", &d_dev);
		else if (memcmp(line, "voip=", 5) == 0)
			sscanf(&line[5], "%d", &d_voip);
		else if (memcmp(line, "dest=", 5) == 0)
			sstring(&line[5], d_dest);
		else if (memcmp(line, "src=", 4) == 0)
			sstring(&line[4], d_src);
		else if (memcmp(line, "down=", 5) == 0)
			sstring(&line[5], d_down);
		else if (memcmp(line, "up=", 3) == 0)
			sstring(&line[3], d_up);
		else if (memcmp(line, "rem12=", 6) == 0)
			sstring(&line[6], d_rem12);
		else if (memcmp(line, "rem34=", 6) == 0)
			sstring(&line[6], d_rem34);
	}
	fclose(fp1);

	d_ft &= 0x7;
	d_mr &= 0x7;
	d_sq &= 0x1;
	d_sc &= 0x7f;
	d_dev &= 0x1;

	int dt = (d_fullrate_mode) ? 3 : 2;
	uint8_t tmp80[80];

	if (d_fullrate_mode) {
		generate_fich(d_fich[0], 1, 2, 0, 0, 0, 0, 0, 0, d_dev, d_mr, d_voip, dt, d_sq, d_sc);
	} else {
		generate_fich(d_fich[0], 1, 2, 0, 0, 0, 0, d_ft, 0, d_dev, d_mr, d_voip, dt, d_sq, d_sc);
		generate_fich(d_fich[1], 1, 2, 0, 0, 0, 1, d_ft, 0, d_dev, d_mr, d_voip, dt, d_sq, d_sc);
		generate_fich(d_fich[2], 1, 2, 0, 0, 0, 2, d_ft, 0, d_dev, d_mr, d_voip, dt, d_sq, d_sc);
		generate_fich(d_fich[3], 1, 2, 0, 0, 0, 3, d_ft, 0, d_dev, d_mr, d_voip, dt, d_sq, d_sc);
		generate_fich(d_fich[4], 1, 2, 0, 0, 0, 4, d_ft, 0, d_dev, d_mr, d_voip, dt, d_sq, d_sc);
		generate_fich(d_fich[5], 1, 2, 0, 0, 0, 5, d_ft, 0, d_dev, d_mr, d_voip, dt, d_sq, d_sc);

		unpack_bytes(tmp80, d_dest, 80);
		generate_dch_vd2(d_vd2_dch[0], tmp80);

		unpack_bytes(tmp80, d_src, 80);
		generate_dch_vd2(d_vd2_dch[1], tmp80);

		unpack_bytes(tmp80, d_down, 80);
		generate_dch_vd2(d_vd2_dch[2], tmp80);

		unpack_bytes(tmp80, d_up, 80);
		generate_dch_vd2(d_vd2_dch[3], tmp80);

		unpack_bytes(tmp80, d_rem12, 80);
		generate_dch_vd2(d_vd2_dch[4], tmp80);

		unpack_bytes(tmp80, d_rem34, 80);
		generate_dch_vd2(d_vd2_dch[5], tmp80);

		d_next_fn = 0;
	}
}

void
ysf_tx_sb_impl::forecast(int nof_output_items, gr_vector_int &nof_input_items_reqd)
{
   // each 480-dibit output frame contains five voice code words=800 samples
   const size_t nof_inputs = nof_input_items_reqd.size();
   const int nof_vcw = nof_output_items / 480.0;
   const int nof_samples_reqd = nof_vcw * 5 * 160;
   std::fill(&nof_input_items_reqd[0], &nof_input_items_reqd[nof_inputs], nof_samples_reqd);
}

int 
ysf_tx_sb_impl::general_work (int noutput_items,
			       gr_vector_int &ninput_items,
			       gr_vector_const_void_star &input_items,
			       gr_vector_void_star &output_items)
{

  int nconsumed = 0;
  int16_t *in;
  in = (int16_t *) input_items[0];
  uint8_t *out = reinterpret_cast<uint8_t*>(output_items[0]);
  int nframes=0;
  int16_t frame_vector[8];
  voice_codeword cw(voice_codeword_sz);
  uint8_t ambe_49bit_codeword[49];
  std::vector <bool> interleaved_buf(144);

  for (int n=0;n < (noutput_items/480);n++) {
    // need (at least) five voice codewords worth of samples
    if (ninput_items[0] - nconsumed < 5*160) break;
    memcpy(out, ysf_fs, sizeof(ysf_fs));
    if (d_fullrate_mode) {
      memcpy(out+20, d_fich[0], 100);
    } else {
        d_next_fn = (d_next_fn + 1) % (d_ft+1);
        memcpy(out+20, d_fich[d_next_fn], 100);
    }
    // TODO: would be nice to multithread these 5
    for (int vcw = 0; vcw < 5; vcw++) {
      if (d_fullrate_mode) {
        d_fullrate_encoder.imbe_encode(frame_vector, in);
        imbe_header_encode(cw, frame_vector[0], frame_vector[1], frame_vector[2], frame_vector[3], frame_vector[4], frame_vector[5], frame_vector[6], frame_vector[7]);
        for (int i=0; i<144; i++) {
          interleaved_buf[ysf_permutation[i]] = cw[i];
        }
        bool_to_dibits(out + vcw*72 + 120, interleaved_buf, 72);
      } else {   /* halfrate mode */
        d_halfrate_encoder.encode(in, ambe_49bit_codeword);
        generate_vch_vd2(out + vcw*72 + 120 + 20, ambe_49bit_codeword);
        memcpy(out + vcw*72 + 120,d_vd2_dch[d_next_fn]+vcw*20, 20);
      }
      in += 160;
      nconsumed += 160;
    }
    nframes += 1;
    out += 480;
  }

  // Tell runtime system how many input items we consumed on
  // each input stream.

  if (nconsumed)
    consume_each(nconsumed);

  // Tell runtime system how many output items we produced.
  return (nframes * 480);
}

void
ysf_tx_sb_impl::set_gain_adjust(float gain_adjust) {
	if (d_fullrate_mode)
		d_fullrate_encoder.set_gain_adjust(gain_adjust);
	else
		d_halfrate_encoder.set_gain_adjust(gain_adjust);
}

  } /* namespace op25_repeater */
} /* namespace gr */
