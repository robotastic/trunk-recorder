//smartnet_deinterleave.cc
/* -*- c++ -*- */
/*
 * Copyright 2012 Nick Foster
 *
 * This file is part of gr_smartnet
 *
 * gr_smartnet is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * gr_smartnet is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "smartnet_deinterleave.h"
#include <gnuradio/io_signature.h>
#include <gnuradio/tags.h>

#define VERBOSE 0

/*
 * Create a new instance of smartnet_deinterleave and return
 * a boost shared_ptr.  This is effectively the public constructor.
 */
smartnet_deinterleave_sptr smartnet_make_deinterleave()
{
	return smartnet_deinterleave_sptr (new smartnet_deinterleave ());
}

/*
 * Specify constraints on number of input and output streams.
 * This info is used to construct the input and output signatures
 * (2nd & 3rd args to gr_block's constructor).  The input and
 * output signatures are used by the runtime system to
 * check that a valid number and type of inputs and outputs
 * are connected to this block.  In this case, we accept
 * only 1 input and 1 output.
 */
static const int MIN_IN = 1;    // mininum number of input streams
static const int MAX_IN = 1;    // maximum number of input streams
static const int MIN_OUT = 1;   // minimum number of output streams
static const int MAX_OUT = 1;   // maximum number of output streams

/*
 * The private constructor
 */
smartnet_deinterleave::smartnet_deinterleave ()
	: gr::block ("deinterleave",
	             gr::io_signature::make (MIN_IN, MAX_IN, sizeof (char)),
	             gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (char)))
{
	set_relative_rate((double)(76.0/84.0));
	set_output_multiple(76);
}

/*
 * Our virtual destructor.
 */
smartnet_deinterleave::~smartnet_deinterleave ()
{
	// nothing else required in this example
}

void smartnet_deinterleave::forecast (int noutput_items,
                                      gr_vector_int &ninput_items_required) //estimate number of input samples required for noutput_items samples
{
	int size = (noutput_items * 84) / 76;

	ninput_items_required[0] = size;
}


int
smartnet_deinterleave::general_work (int noutput_items,
                                     gr_vector_int &ninput_items,
                                     gr_vector_const_void_star &input_items,
                                     gr_vector_void_star &output_items)
{
	const char *in = (const char *) input_items[0];
	char *out = (char *) output_items[0];

	if(VERBOSE) std::cout << "Deinterleave called with " << noutput_items << " outputs" << std::endl;

	//you will need to look ahead 84 bits to post 76 bits of data
	//TODO this needs to be able to handle shorter frames while keeping state in order to end gracefully
	int size = ninput_items[0] - 84;
	if(size <= 0) {
		consume_each(0);
		return 0; //better luck next time
	}

	uint64_t abs_sample_cnt = nitems_read(0);
	std::vector<gr::tag_t> preamble_tags;

	uint64_t outlen = 0; //output sample count

	get_tags_in_range(preamble_tags, 0, abs_sample_cnt, abs_sample_cnt + size, pmt::string_to_symbol("smartnet_preamble"));
	if(preamble_tags.size() == 0) {
		consume_each(size);
		return 0;
	}

	std::vector<gr::tag_t>::iterator tag_iter;
	for(tag_iter = preamble_tags.begin(); tag_iter != preamble_tags.end(); tag_iter++) {
		uint64_t mark = tag_iter->offset - abs_sample_cnt;

		if(VERBOSE)
			std::cout << "found a preamble at " << tag_iter->offset << std::endl;

		for(int k=0; k<76/4; k++) {
			for(int l=0; l<4; l++) {
				out[k*4 + l] = in[mark + k + l*19];
			}
		}

		//since you're a nonsynchronized block, you have to reissue a
		//tag with the correct output sample number
		add_item_tag(0, //stream ID
		             nitems_written(0) + mark, //sample
		             pmt::string_to_symbol("smartnet_frame"), //key
		             pmt::pmt_t() //data (unused here)
		            );
		outlen += 76;
	}

	if(VERBOSE) std::cout << "consumed " << size << ", produced " << outlen << std::endl;
	consume_each(preamble_tags.back().offset - abs_sample_cnt + 84);
	return outlen;
}
