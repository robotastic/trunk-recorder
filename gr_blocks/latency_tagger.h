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

#ifndef INCLUDED_LATENCY_TAGGER_H
#define INCLUDED_LATENCY_TAGGER_H

#include "latency_api.h"
#include <gnuradio/sync_block.h>

class latency_tagger;
typedef boost::shared_ptr<latency_tagger> latency_tagger_sptr;

LATENCY_API latency_tagger_sptr latency_make_tagger (int item_size, int tag_frequency, std::string tag);

/*!
 * \brief <+description+>
 *
 */
class LATENCY_API latency_tagger : public gr::sync_block
{
	friend LATENCY_API latency_tagger_sptr latency_make_tagger (int item_size, int tag_frequency, std::string tag);

	latency_tagger (int item_size, int tag_frequency, std::string tag);
    int d_tag_frequency;
    int d_itemsize;
    pmt::pmt_t d_key;
    pmt::pmt_t d_src;

 public:
	~latency_tagger ();

	int work (int noutput_items,
		gr_vector_const_void_star &input_items,
		gr_vector_void_star &output_items);
};

#endif /* INCLUDED_LATENCY_TAGGER_H */
