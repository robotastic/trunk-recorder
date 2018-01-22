/* -*- c++ -*- */
/* 
 * Copyright 2009, 2010, 2011, 2012, 2013 KA1RBI
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

#ifndef INCLUDED_OP25_REPEATER_VOCODER_IMPL_H
#define INCLUDED_OP25_REPEATER_VOCODER_IMPL_H

#include <op25_repeater/vocoder.h>

#include <sys/time.h>
#include <netinet/in.h>
#include <stdint.h>
#include <vector>
#include <deque>

#include "op25_audio.h"
#include "p25p1_voice_encode.h"
#include "p25p1_voice_decode.h"

namespace gr {
  namespace op25_repeater {

    class vocoder_impl : public vocoder
    {
     private:
      // Nothing to declare in this block.

     public:
      vocoder_impl(bool encode_flag, bool verbose_flag, int stretch_amt, char* udp_host, int udp_port, bool raw_vectors_flag);
      ~vocoder_impl();

      void forecast (int noutput_items, gr_vector_int &ninput_items_required);
      void set_gain_adjust(float gain_adjust);

      int general_work(int noutput_items,
		       gr_vector_int &ninput_items,
		       gr_vector_const_void_star &input_items,
		       gr_vector_void_star &output_items);

      int general_work_encode (int noutput_items,
		    gr_vector_int &ninput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
      int general_work_decode (int noutput_items,
		    gr_vector_int &ninput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);

  private:

	std::deque<uint8_t> output_queue;
	std::deque<int16_t> output_queue_decode;
	int opt_udp_port;
	bool opt_encode_flag;
        op25_audio op25audio;
        p25p1_voice_encode p1voice_encode;
        p25p1_voice_decode p1voice_decode;

    };

  } // namespace op25_repeater
} // namespace gr

#endif /* INCLUDED_OP25_REPEATER_VOCODER_IMPL_H */
