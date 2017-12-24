/* -*- c++ -*- */
/*
 * Copyright 2006, 2007 Frank (Radio Rausch)
 * Copyright 2011 Steve Glass
 *
 * This file is part of OP25.
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

#ifndef INCLUDED_OP25_REPEATER_FSK4_DEMOD_FF_IMPL_H
#define INCLUDED_OP25_REPEATER_FSK4_DEMOD_FF_IMPL_H

#include <op25_repeater/fsk4_demod_ff.h>

namespace gr {
  namespace op25_repeater  {

    class fsk4_demod_ff_impl : public fsk4_demod_ff
    {
     private:
      const float d_block_rate;
      boost::scoped_array<float> d_history;
      size_t d_history_last;
      gr::msg_queue::sptr d_queue;
      double d_symbol_clock;
      double d_symbol_spread;
      const float d_symbol_time;
      double fine_frequency_correction;
      double coarse_frequency_correction;

      /**
       * Called when we want the input frequency to be adjusted.
       */
      void send_frequency_correction();

      /**
       * Tracking loop.
       */
      bool tracking_loop_mmse(float input, float *output);

     public:
      fsk4_demod_ff_impl(gr::msg_queue::sptr queue, float sample_rate_Hz, float symbol_rate_Hz);
      ~fsk4_demod_ff_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);
      void reset();
      int general_work(int noutput_items,
		       gr_vector_int &ninput_items,
		       gr_vector_const_void_star &input_items,
		       gr_vector_void_star &output_items);
    };

  } // namespace op25_repeater
} // namespace gr

#endif /* INCLUDED_OP25_FSK4_DEMOD_FF_IMPL_H */
