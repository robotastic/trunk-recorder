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

#ifndef INCLUDED_OP25_REPEATER_FSK4_SLICER_FB_IMPL_H
#define INCLUDED_OP25_REPEATER_FSK4_SLICER_FB_IMPL_H

#include <op25_repeater/fsk4_slicer_fb.h>

namespace gr {
  namespace op25_repeater {

    class fsk4_slicer_fb_impl : public fsk4_slicer_fb
    {
     private:
      float d_slice_levels[4];

     public:
      fsk4_slicer_fb_impl(const std::vector<float> &slice_levels);
      ~fsk4_slicer_fb_impl();

      // Where all the action really happens
      int work(int noutput_items,
	       gr_vector_const_void_star &input_items,
	       gr_vector_void_star &output_items);
    };

  } // namespace op25_repeater
} // namespace gr

#endif /* INCLUDED_OP25_REPEATER_FSK4_SLICER_FB_IMPL_H */

