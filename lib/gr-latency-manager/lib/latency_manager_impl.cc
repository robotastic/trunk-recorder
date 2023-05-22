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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "latency_manager_impl.h"
#include <boost/thread/thread.hpp>

namespace gr {
  namespace latency_manager {

    latency_manager::sptr
    latency_manager::make(int max_tags_in_flight, int tag_interval, int itemsize)
    {
      return gnuradio::get_initial_sptr
        (new latency_manager_impl(max_tags_in_flight, tag_interval, itemsize));
    }

    void latency_manager_impl::add_token(pmt::pmt_t msg)
    {
        d_tokens++;
//        std::cout << "Tokens: " << d_tokens << " : Added one\n";
    }

    latency_manager_impl::latency_manager_impl(int max_tags_in_flight, int tag_interval, int itemsize)
      : gr::sync_block("latency_manager",
              gr::io_signature::make(1, 1, itemsize),
              gr::io_signature::make(1, 1, itemsize)),
        d_itemsize(itemsize),
        d_tag_interval(tag_interval),
        d_tag_phase(0)
    {
        d_tokens = max_tags_in_flight;
        message_port_register_in(pmt::mp("token"));
        set_msg_handler(pmt::mp("token"), [this](pmt::pmt_t msg) { this->add_token(msg); });
        d_tag.key = pmt::intern("latency_strobe");
        d_tag.srcid = alias_pmt();

    }

    latency_manager_impl::~latency_manager_impl()
    {
    }

    int
    latency_manager_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const char *in = (const char*) input_items[0];
      char *out = (char *) output_items[0];

      int copy_count = std::min(noutput_items, d_tag_phase + d_tokens * d_tag_interval);
      std::memcpy(out, in, copy_count * d_itemsize);

      int tag_loc = d_tag_phase;
      while (tag_loc < copy_count) {
        d_tag.offset = nitems_written(0) + tag_loc;
        d_tag.value = pmt::from_long(tag_loc);
        add_item_tag(0,d_tag);
        tag_loc += d_tag_interval;
        d_tokens--;
      }
      d_tag_phase = tag_loc - copy_count;
      if(copy_count == 0) {
        boost::this_thread::sleep(boost::posix_time::microseconds(long(100)));
      }
      return copy_count;
    }
  } /* namespace latency_manager */
} /* namespace gr */

