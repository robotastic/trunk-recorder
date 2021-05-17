/* -*- c++ -*- */
/* 
 * Copyright 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017 Max H. Parke KA1RBI 
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
#include "frame_assembler_impl.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <sys/time.h>

namespace gr {
  namespace op25_repeater {

    void frame_assembler_impl::set_xormask(const char*p) {
    }

    void frame_assembler_impl::set_slotid(int slotid) {
    }

    frame_assembler::sptr
    frame_assembler::make(const char* options, int debug, gr::msg_queue::sptr queue)
    {
      return gnuradio::get_initial_sptr
        (new frame_assembler_impl(options, debug, queue));
    }

    /*
     * The private constructor
     */

    /*
     * Our virtual destructor.
     */
    frame_assembler_impl::~frame_assembler_impl()
    {
    }

static const int MIN_IN = 1;	// mininum number of input streams
static const int MAX_IN = 1;	// maximum number of input streams

/*
 * The private constructor
 */
    frame_assembler_impl::frame_assembler_impl(const char* options, int debug, gr::msg_queue::sptr queue)
      : gr::block("frame_assembler",
		   gr::io_signature::make (MIN_IN, MAX_IN, sizeof (char)),
		   gr::io_signature::make (0, 0, 0)),
	d_msg_queue(queue),
	d_sync(options, debug)
{
}

int 
frame_assembler_impl::general_work (int noutput_items,
                               gr_vector_int &ninput_items,
                               gr_vector_const_void_star &input_items,
                               gr_vector_void_star &output_items)
{

  const uint8_t *in = (const uint8_t *) input_items[0];

  for (int i=0; i<ninput_items[0]; i++)
      d_sync.rx_sym(in[i]);
  consume_each(ninput_items[0]);
  // Tell runtime system how many output items we produced.
  return 0;
}

  } /* namespace op25_repeater */
} /* namespace gr */
