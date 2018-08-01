/* -*- c++ -*- */
/* 
 * DSTAR Encoder (C) Copyright 2017 Max H. Parke KA1RBI
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
#include "dstar_tx_sb_impl.h"
// #include "dstar_const.h"
#include <op25_imbe_frame.h>
#include "dstar_header.h"

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

static inline void sstring(const char s[], char dest[8]) {
	memset(dest, ' ', 8);
	memcpy(dest, s, std::min(strlen(s), (size_t)8));
        for (int i=0; i<8; i++) {
		if (dest[i] < ' ')
			dest [i] = ' ';
	}
}

static const uint8_t FS[24] = { 1,0,1,0,1,0,1,0,1,0,1,1,0,1,0,0,0,1,1,0,1,0,0,0 };
static const uint8_t FS_DUMMY[24] = { 0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1 };

namespace gr {
  namespace op25_repeater {

    dstar_tx_sb::sptr
    dstar_tx_sb::make(int verbose_flag, const char * config_file)
    {
      return gnuradio::get_initial_sptr
        (new dstar_tx_sb_impl(verbose_flag, config_file));
    }

//////////////////////////////////////////////////////////////////////////

static const int MIN_IN = 1;
static const int MAX_IN = 1;

static const int MIN_OUT = 1;
static const int MAX_OUT = 1;

    /*
     * The private constructor
     */
    dstar_tx_sb_impl::dstar_tx_sb_impl(int verbose_flag, const char * config_file)
      : gr::block("dstar_tx_sb",
              gr::io_signature::make (MIN_IN, MAX_IN, sizeof(short)),
              gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof(char))),
              d_verbose_flag(verbose_flag),
              d_config_file(config_file),
              d_frame_counter(0)
    {
      set_output_multiple(96);
      d_encoder.set_dstar_mode();
      config();
    }

    /*
     * Our virtual destructor.
     */
    dstar_tx_sb_impl::~dstar_tx_sb_impl()
    {
    }

void
dstar_tx_sb_impl::config()
{
	FILE * fp1 = fopen(d_config_file, "r");
	char line[256];
	char * cp;
	int flag1, flag2, flag3;
	char rptcall1[8];
	char rptcall2[8];
	char urcall[8];
	char mycall1[8];
	char mycall2[8];

	if (!fp1) {
		fprintf(stderr, "dstar_tx_sb_impl:config: failed to open %s\n", d_config_file);
		return;
	}
	for (;;) {
		cp = fgets(line, sizeof(line) - 2, fp1);
		if (!cp) break;
		if (line[0] == '#') continue;
		if (memcmp(line, "flag1=", 6) == 0)
			sscanf(&line[6], "%x", &flag1);
		else if (memcmp(line, "flag2=", 6) == 0)
			sscanf(&line[6], "%x", &flag2);
		else if (memcmp(line, "flag3=", 6) == 0)
			sscanf(&line[6], "%x", &flag3);
		else if (memcmp(line, "rptcall2=", 9) == 0)
			sstring(&line[9], rptcall2);
		else if (memcmp(line, "rptcall1=", 9) == 0)
			sstring(&line[9], rptcall1);
		else if (memcmp(line, "urcall=", 7) == 0)
			sstring(&line[7], urcall);
		else if (memcmp(line, "mycall1=", 8) == 0)
			sstring(&line[8], mycall1);
		else if (memcmp(line, "mycall2=", 8) == 0)
			sstring(&line[8], mycall2);
	}
	fclose(fp1);
	make_dstar_header(d_dstar_header_data, flag1 & 0xff, flag2 & 0xff, flag3 & 0xff, rptcall2, rptcall1, urcall, mycall1, mycall2);
}

void
dstar_tx_sb_impl::forecast(int nof_output_items, gr_vector_int &nof_input_items_reqd)
{
   // each 96-bit output frame contains one voice code word=160 samples input
   const size_t nof_inputs = nof_input_items_reqd.size();
   const int nof_vcw = nof_output_items / 96.0;
   const int nof_samples_reqd = nof_vcw * 160;
   std::fill(&nof_input_items_reqd[0], &nof_input_items_reqd[nof_inputs], nof_samples_reqd);
}

int 
dstar_tx_sb_impl::general_work (int noutput_items,
			       gr_vector_int &ninput_items,
			       gr_vector_const_void_star &input_items,
			       gr_vector_void_star &output_items)
{

  int nconsumed = 0;
  int16_t *in;
  in = (int16_t *) input_items[0];
  uint8_t *out = reinterpret_cast<uint8_t*>(output_items[0]);
  int nframes=0;

  for (int n=0;n < (noutput_items/96);n++) {
    // need (at least) one voice codeword worth of samples
    if (ninput_items[0] - nconsumed < 160)
      break;
    d_encoder.encode(in, out);
    if (d_frame_counter == 0)
        memcpy(out+72, FS, 24);
    else
        memcpy(out+72, d_dstar_header_data+((d_frame_counter-1) * 24), 24);
    d_frame_counter = (d_frame_counter + 1) % 21;
    in += 160;
    nconsumed += 160;
    nframes += 1;
    out += 96;
  }

  // Tell runtime system how many input items we consumed on
  // each input stream.

  if (nconsumed)
    consume_each(nconsumed);

  // Tell runtime system how many output items we produced.
  return (nframes * 96);
}

void
dstar_tx_sb_impl::set_gain_adjust(float gain_adjust) {
	d_encoder.set_gain_adjust(gain_adjust);
}
  } /* namespace op25_repeater */
} /* namespace gr */
