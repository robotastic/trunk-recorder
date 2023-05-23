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

#ifndef INCLUDED_LATENCY_PROBE_H
#define INCLUDED_LATENCY_PROBE_H


#include <map>
#include <gnuradio/sync_block.h>
#include "boost/tuple/tuple.hpp" 


#include <gnuradio/attributes.h>

#ifdef gnuradio_latency_probe_EXPORTS
#define LATENCY_PROBE_API __GR_ATTR_EXPORT
#else
#define LATENCY_PROBE_API __GR_ATTR_IMPORT
#endif

namespace gr {
  namespace gr_latency {

    /*!
     * \brief <+description of block+>
     * \ingroup latency_manager
     *
     */


typedef struct  {
    uint64_t offset;
    double delay;
    double t_start;
    double t_end;
} latmes_t;

/*!
 * \brief <+description+>
 *
 */

class LATENCY_PROBE_API latency_probe : public gr::sync_block
{


	latency_probe (int item_size, std::vector<std::string> keys);
    std::vector<pmt::pmt_t> d_keys;
    int d_itemsize;
    std::map< pmt::pmt_t, std::vector< latmes_t  > > d_measurements;
 public:
     typedef std::shared_ptr<latency_probe> sptr;
    static sptr make(int item_size, std::vector<std::string> keys);
	~latency_probe ();
	int work (int noutput_items,
		gr_vector_const_void_star &input_items,
		gr_vector_void_star &output_items);

    std::vector<std::string> get_keys();
    std::vector<unsigned long> get_offsets(std::string key);
    std::vector<double> get_delays(std::string key);
    std::vector<double> get_t_start(std::string key);
    std::vector<double> get_t_end(std::string key);

    void reset();
};
  }
}
#endif /* INCLUDED_LATENCY_PROBE_H */
