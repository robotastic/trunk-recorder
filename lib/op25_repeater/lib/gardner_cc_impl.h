/* -*- c++ -*- */
/* 
 * Copyright 2005,2006,2007 Free Software Foundation, Inc.
 *
 * Gardner symbol recovery block for GR - Copyright 2010, 2011, 2012, 2013 KA1RBI
 * 
 * This file is part of OP25 and part of GNU Radio
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

#ifndef INCLUDED_OP25_REPEATER_GARDNER_CC_IMPL_H
#define INCLUDED_OP25_REPEATER_GARDNER_CC_IMPL_H

#include <op25_repeater/gardner_cc.h>

#include <gnuradio/gr_complex.h>
#include <gnuradio/math.h>

#include <gnuradio/filter/mmse_fir_interpolator_cc.h>
// class gr::filter::mmse_fir_interpolator_cc;

namespace gr {
    namespace op25_repeater {

// Integrate & Dump Averager - maintains queue of N samples
// New samples added to accumulator total and old samples subtracted as they drop off
class id_avg {
    public:
        inline id_avg(int N) { d_N = N; d_accum = 0; }
        inline ~id_avg() { }
        inline void reset() { d_queue.clear(); d_accum = 0; }
        inline float avg() { if (d_queue.empty()) return 0; return d_accum / (2 * d_queue.size()); }
        inline void add(const float new_value) {
            d_accum += new_value; d_queue.push_front(new_value);
            if ((d_queue.size() > d_N) && !d_queue.empty()) {
                d_accum -= d_queue.back();
                d_queue.pop_back();
            }
        }

    private:
        size_t d_N;
        float d_accum;
        std::deque<float> d_queue;
};

// Gardner Timing Recovery
class gardner_cc_impl : public gardner_cc
{
    public:
        gardner_cc_impl(float samples_per_symbol, float gain_mu, float gain_omega, float lock_threshold);

        /*!
         * \brief Gardner based repeater gardner_cc block with complex input, complex output.
         * \ingroup sync_blk
         *
         * This implements a Gardner discrete-time error-tracking synchronizer.
         *
         * input samples should be within normalized range of -1.0 thru +1.0
         *
         * includes some simplifying approximations KA1RBI
         */
        ~gardner_cc_impl();

        // Where all the action really happens
        void forecast (int noutput_items, gr_vector_int &ninput_items_required);

        int general_work(int noutput_items,
                         gr_vector_int &ninput_items,
                         gr_vector_const_void_star &input_items,
                         gr_vector_void_star &output_items);
        void set_verbose (bool verbose) { d_verbose = verbose; }

        //! Sets value of omega and its min and max values 
        void set_omega (float omega);
        void reset();
        bool locked() { return (d_lock_accum.avg() >= d_lock_threshold ? true : false); }
        float quality() { return d_lock_accum.avg(); }

    protected:
        bool input_sample0(gr_complex, gr_complex& outp);
        bool input_sample(gr_complex, gr_complex& outp);

    private:
        float d_mu;
        float d_omega, d_gain_omega, d_omega_rel, d_max_omega, d_min_omega, d_omega_mid;
        float d_gain_mu;
        float d_lock_threshold;
        id_avg d_lock_accum;

        gr_complex d_last_sample;
        gr::filter::mmse_fir_interpolator_cc *d_interp;
        bool d_verbose;

        gr_complex *d_dl;
        int         d_dl_index;

        int         d_twice_sps;           
        float       d_timing_error;

        uint32_t    d_interp_counter;

        float       d_theta;
        float       d_phase;

        uint64_t    nid_accum;

        gr_complex  d_prev;
        int         d_event_count;
        char        d_event_type;
        int         d_symbol_seq;
        int         d_update_request;
        float       d_fm;
        float       d_fm_accum;
        int         d_fm_count;
};


    } // namespace op25_repeater
} // namespace gr

#endif /* INCLUDED_OP25_REPEATER_GARDNER_CC_IMPL_H */
