/* -*- c++ -*- */
/*
 * Copyright 2005,2010,2013 Free Software Foundation, Inc.
 * Copyright 2020 Graham J. Norbury, gnorbury@bondcar.com
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
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

#include "rmsagc_ff_impl.h"
#include <gnuradio/io_signature.h>
#include <cmath>

namespace gr {
    namespace op25_repeater {

        rmsagc_ff::sptr
        rmsagc_ff::make(double alpha, double k)
        {
            return gnuradio::get_initial_sptr
                (new rmsagc_ff_impl(alpha, k));
        }

        rmsagc_ff_impl::rmsagc_ff_impl(double alpha, double k)
            : sync_block("rmsagc_ff",
                    io_signature::make(1, 1, sizeof(float)),
                    io_signature::make(1, 1, sizeof(float)))
        {
            set_k(k);
            set_alpha(alpha);
        }

        rmsagc_ff_impl::~rmsagc_ff_impl()
        {
        }

        void
        rmsagc_ff_impl::set_alpha(double alpha)
        {
            d_alpha = alpha;
            d_beta = 1 - d_alpha;
            d_avg = 1.0;
        }

        void
        rmsagc_ff_impl::set_k(double k)
        {
            d_gain = k;
        }

        int
        rmsagc_ff_impl::work(int noutput_items,
                gr_vector_const_void_star &input_items,
                gr_vector_void_star &output_items)
        {
            const float *in = (const float *)input_items[0];
            float *out = (float *)output_items[0];

            for(int i = 0; i < noutput_items; i++) {
                double mag_sqrd = in[i]*in[i];
                d_avg = d_beta*d_avg + d_alpha*mag_sqrd;
                if (d_avg > 0)
                    out[i] = d_gain * in[i] / sqrt(d_avg);
                else
                    out[i] = d_gain * in[i];
            }

            return noutput_items;
        }

} /* namespace op25_repeater */
} /* namespace gr */
