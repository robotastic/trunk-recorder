/* -*- c++ -*- */
/*
 * Copyright 2005,2013 Free Software Foundation, Inc.
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

#ifndef INCLUDED_OP25_REPEATER_RMSAGC_FF_H
#define INCLUDED_OP25_REPEATER_RMSAGC_FF_H

#include <op25_repeater/api.h>
#include <gnuradio/blocks/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
    namespace op25_repeater {

        /*!
         * \brief RMS average power
         * \ingroup math_operators_blk
         */
        class OP25_REPEATER_API rmsagc_ff : virtual public gr::sync_block
        {
            public:
                // gr::blocks::rmsagc_ff::sptr
                #if GNURADIO_VERSION < 0x030900
                typedef boost::shared_ptr<rmsagc_ff> sptr;
                #else
                typedef std::shared_ptr<rmsagc_ff> sptr;
                #endif
                /*!
                 * \brief Make an RMS calc. block.
                 * \param alpha gain for running average filter.
                 */
                static sptr make(double alpha = 0.01, double k = 1.0);

                virtual void set_alpha(double alpha) = 0;
                virtual void set_k(double k) = 0;
        };

    } /* namespace op25_repeater */
} /* namespace gr */

#endif /* INCLUDED_OP25_REPEATER_RMSAGC_FF_H */
