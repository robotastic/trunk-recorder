/* -*- c++ -*- */
/* 
 * GNU Radio interface for halfrate ambe encoder
 * 
 * (C) Copyright 2016 Max H. Parke KA1RBI
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
#include "ambe_encoder_sb_impl.h"

#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace gr {
  namespace op25_repeater {

    ambe_encoder_sb::sptr
    ambe_encoder_sb::make(int verbose_flag)
    {
      return gnuradio::get_initial_sptr
        (new ambe_encoder_sb_impl(verbose_flag));
    }

//////////////////////////////////////////////////////////////////////////
// accepts buffers of 160 samples (short format)
// outputs one ambe codeword per 160 samples (36 dibits, char format)
static const int MIN_IN = 1;
static const int MIN_OUT = 1;
static const int MAX_IN = 1;
static const int MAX_OUT = 1;

    /*
     * The private constructor
     */
    ambe_encoder_sb_impl::ambe_encoder_sb_impl(int verbose_flag)
      : gr::block("ambe_encoder_sb",
              gr::io_signature::make (MIN_IN, MAX_IN, sizeof(short)),
              gr::io_signature::make (MIN_OUT, MAX_OUT, 36))
    {
      set_history(160);
    }

    /*
     * Our virtual destructor.
     */
    ambe_encoder_sb_impl::~ambe_encoder_sb_impl()
    {
    }

void
ambe_encoder_sb_impl::forecast(int nof_output_items, gr_vector_int &nof_input_items_reqd)
{
   /* produces 1 36-byte-block sample per 160 samples input
    */
   const size_t nof_inputs = nof_input_items_reqd.size();
   const int nof_samples_reqd = 160.0 * (nof_output_items);
   std::fill(&nof_input_items_reqd[0], &nof_input_items_reqd[nof_inputs], nof_samples_reqd);
}

int 
ambe_encoder_sb_impl::general_work (int noutput_items,
			       gr_vector_int &ninput_items,
			       gr_vector_const_void_star &input_items,
			       gr_vector_void_star &output_items)
{
  int nframes = ninput_items[0] / 160;
  nframes = std::min(nframes, noutput_items);
  if (nframes < 1) return 0;
  short *in = (short *) input_items[0];
  uint8_t *out = reinterpret_cast<uint8_t*>(output_items[0]);

  for (int n=0; n<nframes; n++) {
    d_encoder.encode(&in[n*160], &out[n*36]);
  }

  // Tell runtime system how many input items we consumed on
  // each input stream.

  consume_each (nframes * 160);

  // Tell runtime system how many output items we produced.
  return (nframes);
}

void
ambe_encoder_sb_impl::set_gain_adjust(float gain_adjust) {
	d_encoder.set_gain_adjust(gain_adjust);
}

  } /* namespace op25_repeater */
} /* namespace gr */
