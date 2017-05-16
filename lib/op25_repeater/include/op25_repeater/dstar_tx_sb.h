/* -*- c++ -*- */
/* 
 * DMR Encoder (C) Copyright 2017 Max H. Parke KA1RBI
 * This file is part of OP25
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


#ifndef INCLUDED_OP25_REPEATER_DSTAR_TX_SB_H
#define INCLUDED_OP25_REPEATER_DSTAR_TX_SB_H

#include <op25_repeater/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace op25_repeater {

    /*!
     * \brief <+description of block+>
     * \ingroup op25_repeater
     *
     */
    class OP25_REPEATER_API dstar_tx_sb : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<dstar_tx_sb> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of op25_repeater::dstar_tx_sb.
       *
       * To avoid accidental use of raw pointers, op25_repeater::dstar_tx_sb's
       * constructor is in a private implementation
       * class. op25_repeater::dstar_tx_sb::make is the public interface for
       * creating new instances.
       */
      static sptr make(int versbose_flag, const char * conf_file);
    };

  } // namespace op25_repeater
} // namespace gr

#endif /* INCLUDED_OP25_REPEATER_DSTAR_TX_SB_H */

