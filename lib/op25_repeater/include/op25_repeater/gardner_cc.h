/* -*- c++ -*- */
/* 
 * Copyright 2013 <+YOU OR YOUR COMPANY+>.
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


#ifndef INCLUDED_OP25_REPEATER_GARDNER_CC_H
#define INCLUDED_OP25_REPEATER_GARDNER_CC_H

#include <op25_repeater/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace op25_repeater {

    /*!
     * \brief <+description of block+>
     * \ingroup op25_repeater
     *
     */
    class OP25_REPEATER_API gardner_cc : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<gardner_cc> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of op25_repeater::gardner_cc.
       *
       * To avoid accidental use of raw pointers, op25_repeater::gardner_cc's
       * constructor is in a private implementation
       * class. op25_repeater::gardner_cc::make is the public interface for
       * creating new instances.
       */
      static sptr make(float samples_per_symbol,
                       float gain_mu,
                       float gain_omega,
                       float lock_threshold = 0.28);
      virtual void set_omega(float omega) {}
      virtual void reset() {}
      virtual bool locked() { return false; }
      virtual float quality() { return 0; }
    };

  } // namespace op25_repeater
} // namespace gr

#endif /* INCLUDED_OP25_REPEATER_GARDNER_CC_H */

