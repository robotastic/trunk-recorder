//* -*- c++ -*- */
/*
 * Copyright 2006,2010-2012 Free Software Foundation, Inc.
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

#include "costas_loop_cc_impl.h"
#include <gnuradio/expj.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/math.h>
#include <gnuradio/sincos.h>
#include <boost/format.hpp>

static const float      M_TWOPI = 2.0f * M_PI;
static const gr_complex PT_45   = gr_expj( M_PI / 4.0 );

namespace gr {
namespace op25_repeater {

costas_loop_cc::sptr costas_loop_cc::make(float loop_bw, int order, float max_phase = M_TWOPI)
{
    return gnuradio::get_initial_sptr(new costas_loop_cc_impl(loop_bw, order, max_phase));
}

costas_loop_cc_impl::costas_loop_cc_impl(float loop_bw, int order, float max_phase = M_TWOPI)
    : sync_block("costas_loop_cc",
                 io_signature::make(1, 1, sizeof(gr_complex)),
                 io_signature::make(1, 1, sizeof(gr_complex))),
      d_order(order),
      d_error(0),
      d_odd_even(0),
      d_phase(0), d_freq(0),
      d_max_freq(1.0), d_min_freq(-1.0),
      d_max_phase(max_phase), d_min_phase(-max_phase),
      d_phase_detector(NULL)
{
    // Set the damping factor for a critically damped system
    d_damping = sqrtf(2.0f) / 2.0f;

    // Set the bandwidth, which will then call update_gains()
    set_loop_bandwidth(loop_bw);

    // Set up the phase detector to use based on the constellation order
    switch (d_order) {
    case 2:
        d_phase_detector = &costas_loop_cc_impl::phase_detector_2;
        break;

    case 4:
        d_phase_detector = &costas_loop_cc_impl::phase_detector_4;
        break;

    case 8:
        d_phase_detector = &costas_loop_cc_impl::phase_detector_8;
        break;

    default:
        throw std::invalid_argument("order must be 2, 4, or 8");
        break;
    }
}

costas_loop_cc_impl::~costas_loop_cc_impl() {}

float costas_loop_cc_impl::phase_detector_8(gr_complex sample) const
{
    /* This technique splits the 8PSK constellation into 2 squashed
       QPSK constellations, one when I is larger than Q and one
       where Q is larger than I. The error is then calculated
       proportionally to these squashed constellations by the const
       K = sqrt(2)-1.

       The signal magnitude must be > 1 or K will incorrectly bias
       the error value.

       Ref: Z. Huang, Z. Yi, M. Zhang, K. Wang, "8PSK demodulation for
       new generation DVB-S2", IEEE Proc. Int. Conf. Communications,
       Circuits and Systems, Vol. 2, pp. 1447 - 1450, 2004.
    */

    float K = (sqrt(2.0) - 1);
    if (fabsf(sample.real()) >= fabsf(sample.imag())) {
        return ((sample.real() > 0 ? 1.0 : -1.0) * sample.imag() -
                (sample.imag() > 0 ? 1.0 : -1.0) * sample.real() * K);
    } else {
        return ((sample.real() > 0 ? 1.0 : -1.0) * sample.imag() * K -
                (sample.imag() > 0 ? 1.0 : -1.0) * sample.real());
    }
}

float costas_loop_cc_impl::phase_detector_4(gr_complex sample) const
{
    return ((sample.real() > 0 ? 1.0 : -1.0) * sample.imag() -
            (sample.imag() > 0 ? 1.0 : -1.0) * sample.real());
}

float costas_loop_cc_impl::phase_detector_2(gr_complex sample) const
{
    return (sample.real() * sample.imag());
}

float costas_loop_cc_impl::error() const { return d_error; }

int costas_loop_cc_impl::work(int noutput_items,
                              gr_vector_const_void_star& input_items,
                              gr_vector_void_star& output_items)
{
    const gr_complex* iptr = (gr_complex*)input_items[0];
    gr_complex* optr = (gr_complex*)output_items[0];
    gr_complex nco_out;

    std::vector<tag_t> tags;
    get_tags_in_range(tags,
                      0,
                      nitems_read(0),
                      nitems_read(0) + noutput_items,
                      pmt::intern("phase_est"));

    for (int i = 0; i < noutput_items; i++) {
        if (!tags.empty()) {
            if (tags[0].offset - nitems_read(0) == (size_t)i) {
                d_phase = (float)pmt::to_double(tags[0].value);
                tags.erase(tags.begin());
            }
        }

        nco_out = gr_expj(-d_phase);
        optr[i] = iptr[i] * nco_out;

        //d_error = (*this.*d_phase_detector)((d_odd_even) ? (optr[i] * PT_45) : (optr[i]));
        d_error = (*this.*d_phase_detector)(optr[i]);
        d_error = gr::branchless_clip(d_error, 1.0);

        advance_loop(d_error);
        //phase_wrap();
        phase_limit();
        frequency_limit();
        d_odd_even ^= 1;
    }

    return noutput_items;
}
void costas_loop_cc_impl::update_gains()
{
    float denom = (1.0 + 2.0 * d_damping * d_loop_bw + d_loop_bw * d_loop_bw);
    d_alpha = (4 * d_damping * d_loop_bw) / denom;
    d_beta = (4 * d_loop_bw * d_loop_bw) / denom;
}

void costas_loop_cc_impl::advance_loop(float error)
{
    d_freq = d_freq + d_beta * error;
    d_phase = d_phase + d_freq + d_alpha * error;
}

void costas_loop_cc_impl::phase_wrap()
{
    while (d_phase > d_max_phase)
        d_phase -= d_max_phase;
    while (d_phase < d_min_phase)
        d_phase -= d_min_phase;
}

void costas_loop_cc_impl::phase_limit()
{
    if (d_phase > d_max_phase)
        d_phase = d_max_phase;
    else if (d_phase < d_min_phase)
        d_phase = d_min_phase;
}

void costas_loop_cc_impl::frequency_limit()
{
    if (d_freq > d_max_freq)
        d_freq = d_max_freq;
    else if (d_freq < d_min_freq)
        d_freq = d_min_freq;
}

/*******************************************************************
 * SET FUNCTIONS
 *******************************************************************/

void costas_loop_cc_impl::set_loop_bandwidth(float bw)
{
    if (bw < 0) {
        throw std::out_of_range("control_loop: invalid bandwidth. Must be >= 0.");
    }

    d_loop_bw = bw;
    update_gains();
}

void costas_loop_cc_impl::set_damping_factor(float df)
{
    if (df <= 0) {
        throw std::out_of_range("control_loop: invalid damping factor. Must be > 0.");
    }

    d_damping = df;
    update_gains();
}

void costas_loop_cc_impl::set_alpha(float alpha)
{
    if (alpha < 0 || alpha > 1.0) {
        throw std::out_of_range("control_loop: invalid alpha. Must be in [0,1].");
    }
    d_alpha = alpha;
}

void costas_loop_cc_impl::set_beta(float beta)
{
    if (beta < 0 || beta > 1.0) {
        throw std::out_of_range("control_loop: invalid beta. Must be in [0,1].");
    }
    d_beta = beta;
}

void costas_loop_cc_impl::set_frequency(float freq)
{
    d_freq = freq;
    frequency_limit();
}

void costas_loop_cc_impl::set_phase(float phase)
{
    d_phase = phase;
    //phase_wrap();
    phase_limit();
}

void costas_loop_cc_impl::set_max_phase(float phase)
{
    d_max_phase = phase;
    d_min_phase = -phase;
    //phase_wrap();
    phase_limit();
}

void costas_loop_cc_impl::set_max_freq(float freq)
{
    d_max_freq = freq;
    frequency_limit();
}

void costas_loop_cc_impl::set_min_freq(float freq)
{
    d_min_freq = freq;
    frequency_limit();
}

/*******************************************************************
 * GET FUNCTIONS
 *******************************************************************/

float costas_loop_cc_impl::get_loop_bandwidth() const { return d_loop_bw; }
float costas_loop_cc_impl::get_damping_factor() const { return d_damping; }
float costas_loop_cc_impl::get_alpha() const { return d_alpha; }
float costas_loop_cc_impl::get_beta() const { return d_beta; }
float costas_loop_cc_impl::get_frequency() const { return d_freq; }
float costas_loop_cc_impl::get_phase() const { return d_phase; }
float costas_loop_cc_impl::get_max_freq() const { return d_max_freq; }
float costas_loop_cc_impl::get_min_freq() const { return d_min_freq; }

} /* namespace op25_repeater */
} /* namespace gr */
