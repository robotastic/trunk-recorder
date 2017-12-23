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

#ifndef INCLUDED_OP25_REPEATER_DMR_BS_TX_BB_IMPL_H
#define INCLUDED_OP25_REPEATER_DMR_BS_TX_BB_IMPL_H

#include <op25_repeater/dmr_bs_tx_bb.h>

#include <sys/time.h>
#include <netinet/in.h>
#include <stdint.h>
#include <vector>
#include <deque>

namespace gr {
  namespace op25_repeater {

    class dmr_bs_tx_bb_impl : public dmr_bs_tx_bb
    {
     private:
     void config(void);

     public:
      dmr_bs_tx_bb_impl(int verbose_flag, const char * config_file);
      ~dmr_bs_tx_bb_impl();

      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
		       gr_vector_int &ninput_items,
		       gr_vector_const_void_star &input_items,
		       gr_vector_void_star &output_items);

  private:
	int d_verbose_flag;
        const char * d_config_file;
	int d_en[2];
	int d_ts[2];
	int d_cc[2];
	int d_so[2];
	int d_ga[2];
	int d_sa[2];
	int d_next_burst;
	int d_next_slot;
	uint8_t d_cach[4][12];
	uint8_t d_embedded[2][4*24];
    };

  } // namespace op25_repeater
} // namespace gr

#endif /* INCLUDED_OP25_REPEATER_DMR_BS_TX_BB_IMPL_H */
