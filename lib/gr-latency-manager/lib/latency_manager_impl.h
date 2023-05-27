/* -*- c++ -*- */
/*
 * Copyright 2019 Derek Kozel.
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

#ifndef INCLUDED_LATENCY_MANAGER_LATENCY_MANAGER_IMPL_H
#define INCLUDED_LATENCY_MANAGER_LATENCY_MANAGER_IMPL_H

#include "../include/latency_manager.h"

namespace gr {
  namespace latency_manager {

    class latency_manager_impl : public latency_manager
    {
     private:
        int d_tokens;
        void add_token(pmt::pmt_t tag);
        int d_itemsize;
        int d_tag_interval;
        int d_tag_phase;
        tag_t d_tag;
        
     public:
      latency_manager_impl(int max_tags_in_flight, int tag_interval, int itemsize);
      ~latency_manager_impl();

      // Where all the action really happens
      int work(
              int noutput_items,
              gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items
      );
    };

  } // namespace latency_manager
} // namespace gr

#endif /* INCLUDED_LATENCY_MANAGER_LATENCY_MANAGER_IMPL_H */

