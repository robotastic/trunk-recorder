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


#ifndef INCLUDED_OP25_REPEATER_P25_FRAME_ASSEMBLER_H
#define INCLUDED_OP25_REPEATER_P25_FRAME_ASSEMBLER_H

#include <op25_repeater/rx_status.h>
#include <op25_repeater/api.h>
#include <gnuradio/block.h>
#include <gnuradio/msg_queue.h>

namespace gr {
  namespace op25_repeater {

    /*!
     * \brief <+description of block+>
     * \ingroup op25_repeater
     *
     */
    class OP25_REPEATER_API p25_frame_assembler : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<p25_frame_assembler> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of op25_repeater::p25_frame_assembler.
       *
       * To avoid accidental use of raw pointers, op25_repeater::p25_frame_assembler's
       * constructor is in a private implementation
       * class. op25_repeater::p25_frame_assembler::make is the public interface for
       * creating new instances.
       */
      virtual void clear() {};
      virtual  void clear_silence_frame_count() {}
      static sptr make(int sys_num, int silence_frames, const char* udp_host, int port, int debug, bool do_imbe, bool do_output, bool do_msgq, gr::msg_queue::sptr queue, bool do_audio_output, bool do_phase2_tdma, bool do_nocrypt);
      virtual void set_xormask(const char*p) {}
      virtual void set_slotid(int slotid) {}
      virtual void set_phase2_tdma(bool p) {}
      virtual void reset_rx_status() {}
      virtual Rx_Status get_rx_status() {Rx_Status rx_status; return rx_status; }
    };

  } // namespace op25_repeater
} // namespace gr

#endif /* INCLUDED_OP25_REPEATER_P25_FRAME_ASSEMBLER_H */
