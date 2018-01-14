/* -*- c++ -*- */
/* 
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

#ifndef INCLUDED_OP25_REPEATER_AMBE_ENCODER_SB_IMPL_H
#define INCLUDED_OP25_REPEATER_AMBE_ENCODER_SB_IMPL_H

#include <op25_repeater/ambe_encoder_sb.h>

#include <sys/time.h>
#include <netinet/in.h>
#include <stdint.h>
#include <vector>
#include <deque>

#include "ambe3600x2250_const.h"
#include "mbelib.h"
#include "ambe.h"
#include "p25p2_vf.h"
#include "imbe_vocoder/imbe_vocoder.h"
#include "ambe_encoder.h"

namespace gr {
  namespace op25_repeater {

    class ambe_encoder_sb_impl : public ambe_encoder_sb
    {
     private:
      // Nothing to declare in this block.

     public:
      ambe_encoder_sb_impl(int verbose_flag);
      ~ambe_encoder_sb_impl();

      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

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
      void set_gain_adjust(float gain_adjust);

  private:
	int d_verbose_flag;
	ambe_encoder d_encoder;
    };

  } // namespace op25_repeater
} // namespace gr

#endif /* INCLUDED_OP25_REPEATER_AMBE_ENCODER_SB_IMPL_H */
