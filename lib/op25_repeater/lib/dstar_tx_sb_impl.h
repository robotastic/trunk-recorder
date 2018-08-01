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

#ifndef INCLUDED_OP25_REPEATER_DSTAR_TX_SB_IMPL_H
#define INCLUDED_OP25_REPEATER_DSTAR_TX_SB_IMPL_H

#include <op25_repeater/dstar_tx_sb.h>

#include <sys/time.h>
#include <netinet/in.h>
#include <stdint.h>
#include <vector>
#include <deque>
#include <algorithm>

#include "imbe_vocoder/imbe_vocoder.h"
#include "ambe_encoder.h"

namespace gr {
  namespace op25_repeater {

    class dstar_tx_sb_impl : public dstar_tx_sb
    {
     private:
     void config(void);

     public:
      dstar_tx_sb_impl(int verbose_flag, const char * config_file);
      ~dstar_tx_sb_impl();

      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
		       gr_vector_int &ninput_items,
		       gr_vector_const_void_star &input_items,
		       gr_vector_void_star &output_items);
      void set_gain_adjust(float gain_adjust);

  private:
	int d_verbose_flag;
	const char * d_config_file;
        ambe_encoder d_encoder;
	int d_frame_counter;
	uint8_t d_dstar_header_data[480];
    };

  } // namespace op25_repeater
} // namespace gr

#endif /* INCLUDED_OP25_REPEATER_DSTAR_TX_SB_IMPL_H */
