/* -*- c++ -*- */
/* 
 * Copyright 2012 <+YOU OR YOUR COMPANY+>.
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
#include "./latency_tagger.h"

#include "boost/date_time/local_time/local_time.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

using boost::posix_time::ptime;
using boost::posix_time::time_from_string;
using boost::posix_time::time_duration;
namespace gr {
  namespace gr_latency {

latency_tagger::sptr latency_tagger::make  (int item_size, int tag_frequency, std::string tag)
{
	return gnuradio::get_initial_sptr(new latency_tagger (item_size, tag_frequency, tag));
}


latency_tagger::latency_tagger (int item_size, int tag_frequency, std::string tag)
	: gr::sync_block ("latency_tagger",
		gr::io_signature::make (1, 1, item_size),
		gr::io_signature::make (1, 1, item_size)),
      d_tag_frequency(tag_frequency),
      d_key(pmt::intern(tag)),
      d_src(pmt::intern(name())),
      d_itemsize(item_size)
{
}


latency_tagger::~latency_tagger ()
{
}


int
latency_tagger::work (int noutput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items)
{
    // add time tags where appropriate
    uint64_t start(nitems_written(0));
    for(uint64_t i=start; i<start+noutput_items; i++){
        if(i%d_tag_frequency==0){
            ptime now(boost::posix_time::microsec_clock::universal_time());
            ptime epoch(boost::gregorian::date(1970,1,1));
            time_duration diff = now - epoch;
            double time = diff.total_seconds() + diff.fractional_seconds() * 1e-6;
            add_item_tag(0, i, d_key, pmt::from_double(time), d_src);
            std::cout << "tagged " << i << " with " << time << std::endl;
        }
    }

    // move data and return
    memcpy(output_items[0], input_items[0], noutput_items*d_itemsize);
	return noutput_items;
}
  }
}