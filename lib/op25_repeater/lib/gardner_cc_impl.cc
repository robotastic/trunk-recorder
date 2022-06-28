/* -*- c++ -*- */
/* 
 * Copyright 2005,2006,2007 Free Software Foundation, Inc.
 *
 * Gardner symbol recovery block for GR - Copyright 2010, 2011, 2012, 2013, 2014, 2015 KA1RBI
 *
 * Pared down original functionality to handle timing recovery only
 * Added lock detector based on Yair Linn's research - Copyright 2022 gnorbury@bondcar.com
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "gardner_cc_impl.h"

#include <gnuradio/math.h>
#include <gnuradio/expj.h>
#include <gnuradio/filter/mmse_fir_interpolator_cc.h>
#include <stdexcept>
#include <cstdio>
#include <string.h>

#include "p25_frame.h"
#include "p25p2_framer.h"
#include "check_frame_sync.h"

static const float M_TWOPI = 2 * M_PI;
static const int   NUM_COMPLEX=100;

namespace gr {
    namespace op25_repeater {

static inline std::complex<float> sgn(std::complex<float>c) {
	if (c == std::complex<float>(0.0,0.0))
		return std::complex<float>(0.0, 0.0);
	return c/abs(c);
}

gardner_cc::sptr
gardner_cc::make(float samples_per_symbol, float gain_mu, float gain_omega, float lock_threshold) {
      return gnuradio::get_initial_sptr
        (new gardner_cc_impl(samples_per_symbol, gain_mu, gain_omega, lock_threshold));
}

/*
 * The private constructor
 */
gardner_cc_impl::gardner_cc_impl(float samples_per_symbol, float gain_mu, float gain_omega, float lock_threshold)
                                : gr::block("gardner_cc",
                                            gr::io_signature::make(1, 1, sizeof(gr_complex)),
                                            gr::io_signature::make(1, 1, sizeof(gr_complex))),
    d_mu(0),
    d_gain_omega(gain_omega),
    d_omega_rel(0.002),
    d_gain_mu(gain_mu),
    d_lock_threshold(lock_threshold),
    d_lock_accum(480),                      // detect timing lock based on last 480 symbols TODO: make configurable
    d_last_sample(0), d_interp(new gr::filter::mmse_fir_interpolator_cc()),
    d_verbose(false),
    d_dl(new gr_complex[NUM_COMPLEX]),
    d_dl_index(0),
    d_interp_counter(0),
    d_event_count(0), d_event_type(' '),
    d_symbol_seq(samples_per_symbol * 4800),
    d_update_request(0)
{
        set_omega(samples_per_symbol);
        set_relative_rate (1.0 / d_omega);
        set_history(d_twice_sps);			// ensure extra input is available
}

/*
 * Our virtual destructor.
 */
gardner_cc_impl::~gardner_cc_impl()
{
    delete [] d_dl;
    delete d_interp;
}

void
gardner_cc_impl::reset()
{
    d_phase = 0;
    d_update_request = 0;
    d_last_sample = 0;
    d_lock_accum.reset();
}

void
gardner_cc_impl::set_omega (float omega)
{
    assert (omega >= 2.0);
    d_omega = omega;
    d_min_omega = omega*(1.0 - d_omega_rel);
    d_max_omega = omega*(1.0 + d_omega_rel);
    d_omega_mid = 0.5*(d_min_omega+d_max_omega);
    d_twice_sps = 2 * (int) ceilf(d_omega);
    int num_complex = std::max(d_twice_sps*2, 16);
    if (num_complex > NUM_COMPLEX)
        fprintf(stderr, "gardner_cc: warning omega %f size %d exceeds NUM_COMPLEX %d\n", omega, num_complex, NUM_COMPLEX);
    *d_dl = gr_complex(0,0); //memset(d_dl, 0, NUM_COMPLEX * sizeof(gr_complex));
}

void
gardner_cc_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required)
{
    unsigned ninputs = ninput_items_required.size();
    for (unsigned i=0; i < ninputs; i++)
        ninput_items_required[i] =
    (int) ceil((noutput_items * d_omega) + d_interp->ntaps());
}

int
gardner_cc_impl::general_work (int noutput_items,
                                      gr_vector_int &ninput_items,
                                      gr_vector_const_void_star &input_items,
                                      gr_vector_void_star &output_items)
{
    const gr_complex *in = (const gr_complex *) input_items[0];
    gr_complex *out = (gr_complex *) output_items[0];

    int i=0, o=0;

    while((o < noutput_items) && (i < ninput_items[0])) {
        while((d_mu > 1.0) && (i < ninput_items[0]))  {
            d_mu --;
            d_dl[d_dl_index] = (in[i]==in[i]) ? in[i] : gr_complex(0, 0);                // Check for NaN values
            d_dl[d_dl_index + d_twice_sps] = (in[i]==in[i]) ? in[i] : gr_complex(0, 0);  // and set to 0
            d_dl_index ++;
            d_dl_index = d_dl_index % d_twice_sps;
            i++;
        }
    
        if (i < ninput_items[0]) {
            float half_omega = d_omega / 2.0;
            int half_sps = (int) floorf(half_omega);
            float half_mu = d_mu + half_omega - (float) half_sps;
            if (half_mu > 1.0) {
                half_mu -= 1.0;
                half_sps += 1;
            }
            // at this point half_sps represents the whole part, and
            // half_mu the fractional part, of the halfway mark.
            // locate two points, separated by half of one symbol time
            // interp_samp is (we hope) at the optimum sampling point 
            gr_complex interp_samp_mid = d_interp->interpolate(&d_dl[ d_dl_index ], d_mu);
            gr_complex interp_samp = d_interp->interpolate(&d_dl[ d_dl_index + half_sps], half_mu);

            float error_real = (d_last_sample.real() - interp_samp.real()) * interp_samp_mid.real();
            float error_imag = (d_last_sample.imag() - interp_samp.imag()) * interp_samp_mid.imag();
            d_last_sample = interp_samp;	// save for next time
            float symbol_error = error_real + error_imag; // Gardner loop error
            if (std::isnan(symbol_error)) symbol_error = 0.0;
            if (symbol_error < -1.0) symbol_error = -1.0;
            if (symbol_error >  1.0) symbol_error =  1.0;

            // Lock detector, based on research paper presented by Yair Linn
            // IEEE Transactions on Wireless Communications Vol 5, No 2, Feb 2006
            float ie2 = interp_samp.real() * interp_samp.real();
            float io2 = interp_samp_mid.real() * interp_samp_mid.real();
            float qe2 = interp_samp.imag() * interp_samp.imag();
            float qo2 = interp_samp_mid.imag() * interp_samp_mid.imag();
            float yi = ((ie2+io2) != 0) ? (ie2-io2)/(ie2+io2) : 0;
            float yq = ((qe2+qo2) != 0) ? (qe2-qo2)/(qe2+qo2) : 0;
            d_lock_accum.add(yi + yq);

            d_omega = d_omega + (d_gain_omega * symbol_error * abs(interp_samp));           // update omega based on loop error
            d_omega = d_omega_mid + gr::branchless_clip(d_omega-d_omega_mid, d_omega_rel);  // make sure we don't walk away

            d_mu += d_omega + d_gain_mu * symbol_error;                                     // update mu based on loop error

            out[o++] = interp_samp;
        }
    }

    consume_each(i);
    return o;
}

    } /* namespace op25_repeater */
} /* namespace gr */
