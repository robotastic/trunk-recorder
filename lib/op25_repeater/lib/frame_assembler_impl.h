/* -*- c++ -*- */
/* 
 * Copyright 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017 Max H. Parke KA1RBI
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

#ifndef INCLUDED_OP25_REPEATER_FRAME_ASSEMBLER_IMPL_H
#define INCLUDED_OP25_REPEATER_FRAME_ASSEMBLER_IMPL_H

#include <op25_repeater/frame_assembler.h>

#include <gnuradio/msg_queue.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <deque>

#include "rx_sync.h"

typedef std::deque<uint8_t> dibit_queue;

namespace gr {
  namespace op25_repeater {

    class frame_assembler_impl : public frame_assembler
    {
     private:
        int d_debug;
	gr::msg_queue::sptr d_msg_queue;
        rx_sync d_sync;

  // internal functions

    void queue_msg(int duid);
    void set_xormask(const char*p) ;
    void set_slotid(int slotid) ;

 public:

     public:
      frame_assembler_impl(const char* options, int debug, gr::msg_queue::sptr queue);
      ~frame_assembler_impl();

      // Where all the action really happens

      int general_work(int noutput_items,
		       gr_vector_int &ninput_items,
		       gr_vector_const_void_star &input_items,
		       gr_vector_void_star &output_items);
    };

  } // namespace op25_repeater
} // namespace gr

#endif /* INCLUDED_OP25_REPEATER_FRAME_ASSEMBLER_IMPL_H */
