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
#include "./latency_probe.h"

#include "boost/date_time/local_time/local_time.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

using boost::posix_time::ptime;
using boost::posix_time::time_from_string;
using boost::posix_time::time_duration;
namespace gr {
  namespace gr_latency {
    latency_probe::sptr latency_probe::make (int item_size, std::vector<std::string> keys) {
        return gnuradio::get_initial_sptr(new latency_probe (item_size, keys));
    }


latency_probe::latency_probe (int item_size, std::vector<std::string> keys)
	: gr::sync_block ("probe",
		gr::io_signature::make(1,1, item_size),
		gr::io_signature::make (0,1, item_size)),
      d_itemsize(item_size)
{
    for(size_t i=0; i<keys.size(); i++){
        pmt::pmt_t this_key( pmt::intern(keys[i]) );
        d_keys.push_back( this_key );
        d_measurements[ this_key ] =  std::vector< latmes_t >();
        }
}


latency_probe::~latency_probe ()
{
}


int
latency_probe::work (int noutput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items)
{
  
    std::vector<tag_t> tags;
    /*
    get_tags_in_range(tags, 0, nitems_read(0), nitems_read(0) + noutput_items);
    for(int i=0; i<tags.size(); i++){
        for(int j=0; j<d_keys.size(); j++){
            if(pmt::eq(d_keys[j], tags[i].key)){
                // get current time
                ptime now(boost::posix_time::microsec_clock::universal_time());
                ptime epoch(boost::gregorian::date(1970,1,1));
                time_duration diff = now - epoch;
                double time = diff.total_seconds() + diff.fractional_seconds() * 1e-6;
 
                // compute time difference from tag
                double ddiff = time - pmt::to_double(tags[i].value);
                std::cout << " Delay: " << std::setprecision(5) << ddiff << std::endl;
                // store it in our measurement structure
                latmes_t tp = {tags[i].offset, ddiff, pmt::to_double(tags[i].value), time};
                d_measurements[ tags[i].key ].push_back( tp );
                }
            }
        }*/
    get_tags_in_range(tags, 0, nitems_read(0), nitems_read(0) + (uint64_t)noutput_items,pmt::intern("recorder"));
    for(int i=0; i<tags.size(); i++){

                // get current time
                ptime now(boost::posix_time::microsec_clock::universal_time());
                ptime epoch(boost::gregorian::date(1970,1,1));
                time_duration diff = now - epoch;
                double time = diff.total_seconds() + diff.fractional_seconds() * 1e-6;
 
                // compute time difference from tag
                double ddiff = time - pmt::to_double(tags[i].value);
                std::cout << " Delay: " << std::setprecision(5) << ddiff << std::endl;

                }
            
        
	// copy outputs if connected and return
    if(output_items.size() > 0){
        memcpy(output_items[0], input_items[0], noutput_items*d_itemsize);
        }
	return noutput_items;
}

typedef latmes_t lm;
typedef std::vector<lm> lmv;
typedef std::map< pmt::pmt_t, lmv > mt;

std::vector<std::string> latency_probe::get_keys(){
    std::vector<std::string> keys;
    for(mt::iterator i = d_measurements.begin(); i!=d_measurements.end(); i++){
        keys.push_back( pmt::symbol_to_string( (*i).first ) );
    }
    return keys;
}


std::vector<unsigned long> latency_probe::get_offsets(std::string key){
    if(d_measurements.find(pmt::intern(key)) == d_measurements.end())
        throw std::runtime_error("latency_probe::get_offsets() called with invalid key");
    lmv pv = d_measurements[pmt::intern(key)];
    std::vector<unsigned long> offsets(pv.size());
    for(lmv::iterator i = pv.begin(); i != pv.end(); i++){
        offsets.push_back( (*i).offset );
    }
    return offsets;
}

std::vector<double> latency_probe::get_delays(std::string key){
    if(d_measurements.find(pmt::intern(key)) == d_measurements.end())
        throw std::runtime_error("latency_probe::get_delays() called with invalid key");
    lmv pv = d_measurements[pmt::intern(key)];
    std::vector<double> delays(pv.size());
    for(lmv::iterator i = pv.begin(); i != pv.end(); i++){
        delays.push_back( (*i).delay );
    }
    return delays;
}

std::vector<double> latency_probe::get_t_start(std::string key){
    if(d_measurements.find(pmt::intern(key)) == d_measurements.end())
        throw std::runtime_error("latency_probe::get_t_start() called with invalid key");
    lmv pv = d_measurements[pmt::intern(key)];
    std::vector<double> times(pv.size());
    for(lmv::iterator i = pv.begin(); i != pv.end(); i++){
        times.push_back( (*i).t_start );
    }
    return times;
}

std::vector<double> latency_probe::get_t_end(std::string key){
    if(d_measurements.find(pmt::intern(key)) == d_measurements.end())
        throw std::runtime_error("latency_probe::get_t_end() called with invalid key");
    lmv pv = d_measurements[pmt::intern(key)];
    std::vector<double> times(pv.size());
    for(lmv::iterator i = pv.begin(); i != pv.end(); i++){
        times.push_back( (*i).t_end );
    }
    return times;
}

void latency_probe::reset(){
    d_measurements.clear();
}

  } /* namespace latency_probe */
} /* namespace gr */