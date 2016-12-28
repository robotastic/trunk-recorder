/* -*- c++ -*- */
/* 
 * GNU Radio interface for Pavel Yazev's Project 25 IMBE Encoder/Decoder
 * 
 * Copyright 2009 Pavel Yazev E-mail: pyazev@gmail.com
 * Copyright 2009, 2010, 2011, 2012, 2013, 2014 KA1RBI
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
#include "p25p1_voice_decode.h"

#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "imbe_vocoder/imbe_vocoder.h"
#include "p25_frame.h"
#include "op25_imbe_frame.h"

namespace gr {
  namespace op25_repeater {

static void clear_bits(bit_vector& v) {
	for (size_t i=0; i<v.size(); i++) {
		v[i]=0;
	}
}

p25p1_voice_decode::p25p1_voice_decode(bool verbose_flag, const char* udp_host, int udp_port, std::deque<int16_t> &_output_queue) :
	write_sock(0),
	write_bufp(0),
	rxbufp(0),
	output_queue(_output_queue),
	opt_verbose(verbose_flag),
	opt_udp_port(udp_port)
    {
	if (opt_udp_port != 0)
		// remote UDP output
		init_sock(udp_host, opt_udp_port);

	const char *p = getenv("IMBE");
	if (p && strcasecmp(p, "soft") == 0)
		d_software_imbe_decoder = true;
	else
		d_software_imbe_decoder = false;
    }

    /*
     * Our virtual destructor.
     */
    p25p1_voice_decode::~p25p1_voice_decode()
    {
    }

void p25p1_voice_decode::rxframe(const uint32_t u[])
{
	int16_t snd[FRAME];
	int16_t frame_vector[8];
	// decode 88 bits, outputs 160 sound samples (8000 rate)
	if (d_software_imbe_decoder) {
		voice_codeword cw(voice_codeword_sz);
		imbe_header_encode(cw, u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7]);
		software_decoder.decode(cw);
		audio_samples *samples = software_decoder.audio();
		for (int i=0; i < FRAME; i++) {
			if (samples->size() > 0) {
				snd[i] = (int16_t)(samples->front() * 32768.0);
				samples->pop_front();
			} else {
				snd[i] = 0;
			}
		}
	} else {
		for (int i=0; i < 8; i++) {
			frame_vector[i] = u[i];
		}
/* TEST*/	frame_vector[7] >>= 1;
		vocoder.imbe_decode(frame_vector, snd);
	}
	if (opt_udp_port > 0) {
		sendto(write_sock, snd, FRAME * sizeof(int16_t), 0, (struct sockaddr*)&write_sock_addr, sizeof(write_sock_addr));
	} else {
		// add generated samples to output queue
		for (int i = 0; i < FRAME; i++) {
			output_queue.push_back(snd[i]);
		}
	}
}

void p25p1_voice_decode::rxchar(const char* c, int len)
{
	uint32_t u[8];

	for (int i = 0; i < len; i++ ) {
		if (c[i] < ' ') {
			if (c[i] == '\n') {
				rxbuf[rxbufp] = 0;
				sscanf(rxbuf, "%x %x %x %x %x %x %x %x", &u[0], &u[1], &u[2], &u[3], &u[4], &u[5], &u[6], &u[7]);
				rxbufp = 0;
				rxframe(u);
			}
			continue;
		}
		rxbuf[rxbufp++] = c[i];
		if (rxbufp >= RXBUF_MAX) {
			rxbufp = RXBUF_MAX - 1;
		}
	} /* end of for() */
}

void p25p1_voice_decode::init_sock(const char* udp_host, int udp_port)
{
        memset (&write_sock_addr, 0, sizeof(write_sock_addr));
        write_sock = socket(PF_INET, SOCK_DGRAM, 17);   // UDP socket
        if (write_sock < 0) {
                fprintf(stderr, "vocoder: socket: %d\n", errno);
                write_sock = 0;
		return;
        }
        if (!inet_aton(udp_host, &write_sock_addr.sin_addr)) {
                fprintf(stderr, "vocoder: bad IP address\n");
		close(write_sock);
		write_sock = 0;
		return;
	}
        write_sock_addr.sin_family = AF_INET;
        write_sock_addr.sin_port = htons(udp_port);
}

  } /* namespace op25_repeater */
} /* namespace gr */
