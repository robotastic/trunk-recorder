/* -*- c++ -*- */
/*
 * Copyright 2010,2011 Steve Glass
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


#ifndef INCLUDED_OP25_FSK4_DEMOD_FF_H
#define INCLUDED_OP25_FSK4_DEMOD_FF_H

#include <op25_repeater/api.h>
#include <gnuradio/block.h>
#include <gnuradio/msg_queue.h>

namespace gr {
  namespace op25_repeater {

    /*!
     * \brief <+description of block+>
     * \ingroup op25
     *
     */
    class OP25_REPEATER_API fsk4_demod_ff : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<fsk4_demod_ff> sptr;

      /*!
       * \brief Demodulate APCO P25 CF4M signals.
       *
       * op25_fsk4_demod_ff is a GNU Radio block for demodulating APCO
       * P25 CF4M signals. This class expects its input to consist of
       * a 4 level FSK modulated baseband signal. It produces a stream
       * of symbols.
       *
       * All inputs are post FM demodulator and symbol shaping filter
       * data is normalized before being sent to this block so these
       * parameters should not need adjusting even when working on
       * different signals.
       *
       * Nominal levels are -3, -1, +1, and +3.
       */
      static sptr make(gr::msg_queue::sptr queue, float sample_rate_Hz, float symbol_rate_Hz);
      virtual void reset() {};
    };

  } // namespace op25
} // namespace gr

#endif /* INCLUDED_OP25_FSK4_DEMOD_FF_H */
