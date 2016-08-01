/* -*- c++ -*- */
/*
 * Copyright 2010, 2011, 2012, 2013, 2014 Max H. Parke KA1RBI
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

#include "p25p1_fdma.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <sys/time.h>
#include "bch.h"
#include "op25_imbe_frame.h"
#include "p25_frame.h"
#include "p25_framer.h"
#include "rs.h"

namespace gr {
  namespace op25_repeater {

static const int64_t TIMEOUT_THRESHOLD = 1000000;

    p25p1_fdma::~p25p1_fdma()
    {
	if (write_sock > 0)
		close(write_sock);
	delete framer;
    }

static uint16_t crc16(uint8_t buf[], int len) {
        uint32_t poly = (1<<12) + (1<<5) + (1<<0);
        uint32_t crc = 0;
        for(int i=0; i<len; i++) {
                uint8_t bits = buf[i];
                for (int j=0; j<8; j++) {
                        uint8_t bit = (bits >> (7-j)) & 1;
                        crc = ((crc << 1) | bit) & 0x1ffff;
                        if (crc & 0x10000)
                                crc = (crc & 0xffff) ^ poly;
		}
	}
        crc = crc ^ 0xffff;
        return crc & 0xffff;
}

/* translated from p25craft.py Michael Ossmann <mike@ossmann.com>  */
static uint32_t crc32(uint8_t buf[], int len) {	/* length is nr. of bits */
        uint32_t g = 0x04c11db7;
        uint64_t crc = 0;
        for (int i = 0; i < len; i++) {
                crc <<= 1;
                int b = ( buf [i / 8] >> (7 - (i % 8)) ) & 1;
                if (((crc >> 32) ^ b) & 1)
                        crc ^= g;
        }
        crc = (crc & 0xffffffff) ^ 0xffffffff;
        return crc;
}

/* find_min is from wireshark/plugins/p25/packet-p25cai.c */
/* Copyright 2008, Michael Ossmann <mike@ossmann.com>  */
/* return the index of the lowest value in a list */
static int
find_min(uint8_t list[], int len)
{
	int min = list[0];
	int index = 0;
	int unique = 1;
	int i;

	for (i = 1; i < len; i++) {
		if (list[i] < min) {
			min = list[i];
			index = i;
			unique = 1;
		} else if (list[i] == min) {
			unique = 0;
		}
	}
	/* return -1 if a minimum can't be found */
	if (!unique)
		return -1;

	return index;
}

/* count_bits is from wireshark/plugins/p25/packet-p25cai.c */
/* Copyright 2008, Michael Ossmann <mike@ossmann.com>  */
/* count the number of 1 bits in an int */
static int
count_bits(unsigned int n)
{
    int i = 0;
    for (i = 0; n != 0; i++)
        n &= n - 1;
    return i;
}

/* adapted from wireshark/plugins/p25/packet-p25cai.c */
/* Copyright 2008, Michael Ossmann <mike@ossmann.com>  */
/* deinterleave and trellis1_2 decoding */
/* buf is assumed to be a buffer of 12 bytes */
static int
block_deinterleave(bit_vector& bv, unsigned int start, uint8_t* buf)
{
	static const uint16_t deinterleave_tb[] = {
	  0,  1,  2,  3,  52, 53, 54, 55, 100,101,102,103, 148,149,150,151,
	  4,  5,  6,  7,  56, 57, 58, 59, 104,105,106,107, 152,153,154,155,
	  8,  9, 10, 11,  60, 61, 62, 63, 108,109,110,111, 156,157,158,159,
	 12, 13, 14, 15,  64, 65, 66, 67, 112,113,114,115, 160,161,162,163,
	 16, 17, 18, 19,  68, 69, 70, 71, 116,117,118,119, 164,165,166,167,
	 20, 21, 22, 23,  72, 73, 74, 75, 120,121,122,123, 168,169,170,171,
	 24, 25, 26, 27,  76, 77, 78, 79, 124,125,126,127, 172,173,174,175,
	 28, 29, 30, 31,  80, 81, 82, 83, 128,129,130,131, 176,177,178,179,
	 32, 33, 34, 35,  84, 85, 86, 87, 132,133,134,135, 180,181,182,183,
	 36, 37, 38, 39,  88, 89, 90, 91, 136,137,138,139, 184,185,186,187,
	 40, 41, 42, 43,  92, 93, 94, 95, 140,141,142,143, 188,189,190,191,
	 44, 45, 46, 47,  96, 97, 98, 99, 144,145,146,147, 192,193,194,195,
	 48, 49, 50, 51 };

	uint8_t hd[4];
	int b, d, j;
	int state = 0;
	uint8_t codeword;
	uint16_t crc;
	uint32_t crc1;
	uint32_t crc2;

	static const uint8_t next_words[4][4] = {
		{0x2, 0xC, 0x1, 0xF},
		{0xE, 0x0, 0xD, 0x3},
		{0x9, 0x7, 0xA, 0x4},
		{0x5, 0xB, 0x6, 0x8}
	};

	memset(buf, 0, 12);

	for (b=0; b < 98*2; b += 4) {
		codeword = (bv[start+deinterleave_tb[b+0]] << 3) +
		           (bv[start+deinterleave_tb[b+1]] << 2) +
		           (bv[start+deinterleave_tb[b+2]] << 1) +
		            bv[start+deinterleave_tb[b+3]]     ;

		/* try each codeword in a row of the state transition table */
		for (j = 0; j < 4; j++) {
			/* find Hamming distance for candidate */
			hd[j] = count_bits(codeword ^ next_words[state][j]);
		}
		/* find the dibit that matches the most codeword bits (minimum Hamming distance) */
		state = find_min(hd, 4);
		/* error if minimum can't be found */
		if(state == -1)
			return -1;	// decode error, return failure
		/* It also might be nice to report a condition where the minimum is
		 * non-zero, i.e. an error has been corrected.  It probably shouldn't
		 * be a permanent failure, though.
		 *
		 * DISSECTOR_ASSERT(hd[state] == 0);
		 */

		/* append dibit onto output buffer */
		d = b >> 2;	// dibit ctr
		if (d < 48) {
			buf[d >> 2] |= state << (6 - ((d%4) * 2));
		}
	}
	crc = crc16(buf, 12);
	if (crc == 0)
		return 0;	// return OK code
	crc1 = crc32(buf, 8*8);	// try crc32
	crc2 = (buf[8] << 24) + (buf[9] << 16) + (buf[10] << 8) + buf[11];
	if (crc1 == crc2)
		return 0;	// return OK code
	return -2;	// trellis decode OK, but CRC error occurred
}

p25p1_fdma::p25p1_fdma(int sys_id, const char* udp_host, int port, int debug, bool do_imbe, bool do_output, bool do_msgq, gr::msg_queue::sptr queue, std::deque<int16_t> &output_queue, bool do_audio_output) :
	write_bufp(0),
	write_sock(0),
  d_sys_id(sys_id),
	d_udp_host(udp_host),
	d_port(port),
	d_debug(debug),
	d_do_imbe(do_imbe),
	d_do_output(do_output),
	d_do_msgq(do_msgq),
	d_msg_queue(queue),
	output_queue(output_queue),
	framer(new p25_framer()),
	d_do_audio_output(do_audio_output),
	p1voice_decode((debug > 0), udp_host, port, output_queue)
{
	gettimeofday(&last_qtime, 0);
	if (port > 0)
		init_sock(d_udp_host, d_port);
}

void
p25p1_fdma::process_duid(uint32_t const duid, uint32_t const nac, uint8_t const buf[], int const len)
{
	char wbuf[256];
	int p = 0;
	if (!d_do_msgq)
		return;
	if (d_msg_queue->full_p())
		return;
	assert (len+4 <= sizeof(wbuf));
  //wbuf[p++] = (char) (d_sys_id+'0'); // clever way to convert int to char
  //wbuf[p++] = ',';
	wbuf[p++] = (nac >> 8) & 0xff;
	wbuf[p++] = nac & 0xff;
	if (buf) {
		memcpy(&wbuf[p], buf, len);	// copy data
		p += len;
	}
	gr::message::sptr msg = gr::message::make_from_string(std::string(wbuf, p), duid, d_sys_id, 0);
	d_msg_queue->insert_tail(msg);
	gettimeofday(&last_qtime, 0);
//	msg.reset();
}

void
p25p1_fdma::rx_sym (const uint8_t *syms, int nsyms)
{
  struct timeval currtime;
  for (int i1 = 0; i1 < nsyms; i1++){
    if(framer->rx_sym(syms[i1])) {   // complete frame was detected
		if (d_debug >= 10) {
			fprintf (stderr, "NAC 0x%X DUID 0x%X len %u errs %u ", framer->nac, framer->duid, framer->frame_size >> 1, framer->bch_errors);
		}
		if ((framer->duid == 0x03) ||
		 (framer->duid == 0x05) ||
		 (framer->duid == 0x0A) ||
		 (framer->duid == 0x0F)) {
			process_duid(framer->duid, framer->nac, NULL, 0);
		}
		if ((framer->duid == 0x07 || framer->duid == 0x0c)) {
			unsigned int d, b;
			int rc[3];
			bit_vector bv1(720);
			int sizes[3] = {360, 576, 720};
			uint8_t deinterleave_buf[3][12];

			if (framer->frame_size > 720) {
				fprintf(stderr, "warning trunk frame size %u exceeds maximum\n", framer->frame_size);
				framer->frame_size = 720;
			}
			for (d=0, b=0; d < framer->frame_size >> 1; d++) {
				if ((d+1) % 36 == 0)
					continue;	// skip SS
				bv1[b++] = framer->frame_body[d*2];
				bv1[b++] = framer->frame_body[d*2+1];
			}
			for(int sz=0; sz < 3; sz++) {
				if (framer->frame_size >= sizes[sz]) {
					rc[sz] = block_deinterleave(bv1,48+64+sz*196  , deinterleave_buf[sz]);
					if (framer->duid == 0x07 && rc[sz] == 0)
						process_duid(framer->duid, framer->nac, deinterleave_buf[sz], 10);
				}
			}
			// two-block mbt is the only format currently supported
			if (framer->duid == 0x0c
			&& framer->frame_size == 576
			&& rc[0] == 0
			&& rc[1] == 0) {
				// we copy first 10 bytes from first and
				// first 8 from second (removes CRC's)
				uint8_t mbt_block[18];
				memcpy(mbt_block, deinterleave_buf[0], 10);
				memcpy(&mbt_block[10], deinterleave_buf[1], 8);
				process_duid(framer->duid, framer->nac, mbt_block, sizeof(mbt_block));
			}
		}
		if (d_debug >= 10 && framer->duid == 0x00) {
			ProcHDU(framer->frame_body);
		} else if (d_debug > 10 && framer->duid == 0x05) {
			ProcLDU1(framer->frame_body);
		} else if (d_debug >= 10 && framer->duid == 0x0a) {
			ProcLDU2(framer->frame_body);
		} else if (d_debug > 10 && framer->duid == 0x0f) {
			ProcTDU(framer->frame_body);
		}
		if (d_debug >= 10)
			fprintf(stderr, "\n");
		if ((d_do_imbe || d_do_audio_output) && (framer->duid == 0x5 || framer->duid == 0xa)) {  // if voice - ldu1 or ldu2
			for(size_t i = 0; i < nof_voice_codewords; ++i) {
				voice_codeword cw(voice_codeword_sz);
				uint32_t E0, ET;
				uint32_t u[8];
				char s[128];
				imbe_deinterleave(framer->frame_body, cw, i);
				// recover 88-bit IMBE voice code word
				imbe_header_decode(cw, u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7], E0, ET);
				// output one 32-byte msg per 0.020 sec.
				// also, 32*9 = 288 byte pkts (for use via UDP)
				sprintf(s, "%03x %03x %03x %03x %03x %03x %03x %03x\n", u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7]);
				if (d_do_output) {
					if (d_do_audio_output) {
						p1voice_decode.rxframe(u);
					} else {
						for (size_t j=0; j < strlen(s); j++) {
							output_queue.push_back(s[j]);
						}
					}
				}
				if (write_sock > 0) {
					memcpy(&write_buf[write_bufp], s, strlen(s));
					write_bufp += strlen(s);
					if (write_bufp >= 288) { // 9 * 32 = 288
						sendto(write_sock, write_buf, 288, 0, (struct sockaddr *)&write_sock_addr, sizeof(write_sock_addr));
						// FIXME check sendto() rc
						write_bufp = 0;
					}
				}
			}
		} // end of imbe/voice
		if (!d_do_imbe) {
			// pack the bits into bytes, MSB first
			size_t obuf_ct = 0;
			uint8_t obuf[P25_VOICE_FRAME_SIZE/2];
			for (uint32_t i = 0; i < framer->frame_size; i += 8) {
				uint8_t b =
					(framer->frame_body[i+0] << 7) +
					(framer->frame_body[i+1] << 6) +
					(framer->frame_body[i+2] << 5) +
					(framer->frame_body[i+3] << 4) +
					(framer->frame_body[i+4] << 3) +
					(framer->frame_body[i+5] << 2) +
					(framer->frame_body[i+6] << 1) +
					(framer->frame_body[i+7]     );
				obuf[obuf_ct++] = b;
			}
			if (write_sock > 0) {
				sendto(write_sock, obuf, obuf_ct, 0, (struct sockaddr*)&write_sock_addr, sizeof(write_sock_addr));
			}
			if (d_do_output) {
				for (size_t j=0; j < obuf_ct; j++) {
					output_queue.push_back(obuf[j]);
				}
			}
		}
    }  // end of complete frame
  }
  if (d_do_msgq && !d_msg_queue->full_p()) {
    // check for timeout
    gettimeofday(&currtime, 0);
    int64_t diff_usec = currtime.tv_usec - last_qtime.tv_usec;
    int64_t diff_sec  = currtime.tv_sec  - last_qtime.tv_sec ;
    if (diff_usec < 0) {
      diff_usec += 1000000;
      diff_sec  -= 1;
    }
    diff_usec += diff_sec * 1000000;
    if (diff_usec >= TIMEOUT_THRESHOLD) {
      gettimeofday(&last_qtime, 0);
      gr::message::sptr msg = gr::message::make(-1, 0, 0);
      d_msg_queue->insert_tail(msg);
    }
  }
}

void p25p1_fdma::init_sock(const char* udp_host, int udp_port)
{
        memset (&write_sock_addr, 0, sizeof(write_sock_addr));
        write_sock = socket(PF_INET, SOCK_DGRAM, 17);   // UDP socket
        if (write_sock < 0) {
                fprintf(stderr, "op25_imbe_vocoder: socket: %d\n", errno);
                write_sock = 0;
		return;
        }
        if (!inet_aton(udp_host, &write_sock_addr.sin_addr)) {
                fprintf(stderr, "op25_imbe_vocoder: inet_aton: bad IP address\n");
		close(write_sock);
		write_sock = 0;
		return;
	}
        write_sock_addr.sin_family = AF_INET;
        write_sock_addr.sin_port = htons(udp_port);
}

  }  // namespace
}  // namespace
