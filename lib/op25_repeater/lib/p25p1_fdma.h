/* -*- c++ -*- */
/*
 * Copyright 2009, 2010, 2011, 2012, 2013, 2014 Max H. Parke KA1RBI
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

#ifndef INCLUDED_OP25_REPEATER_P25P1_FDMA_H
#define INCLUDED_OP25_REPEATER_P25P1_FDMA_H

#include <gnuradio/msg_queue.h>
#include <sys/time.h>
#include <deque>
#include <math.h>
#include "ezpwd/rs"

#include "log_ts.h"
#include "op25_audio.h"
#include "../include/op25_repeater/rx_status.h"
#include "p25_framer.h"
#include "p25p1_voice_encode.h"
#include "p25p1_voice_decode.h"

namespace gr {
namespace op25_repeater {
    class p25p1_fdma
    {
private:
	typedef std::vector<bool> bit_vector;
	typedef std::array<uint8_t, 12> block_array;
	typedef std::vector<block_array> block_vector;

  // internal functions
	bool header_codeword(uint64_t acc, uint32_t& nac, uint32_t& duid);
	void process_duid(uint32_t const duid, uint32_t const nac, const uint8_t* buf, const int len);
        void process_HDU(const bit_vector& A);
        void process_LCW(std::vector<uint8_t>& HB);
        void process_LLDU(const bit_vector& A, std::vector<uint8_t>& HB);
        void process_LDU1(const bit_vector& A);
        void process_LDU2(const bit_vector& A);
        void process_TTDU();
        void process_TDU15(const bit_vector& A);
        void process_TDU3();
	void process_TSBK(const bit_vector& fr, uint32_t fr_len);
	void process_PDU(const bit_vector& fr, uint32_t fr_len);
	void process_voice(const bit_vector& A);
	int  process_blocks(const bit_vector& fr, uint32_t& fr_len, block_vector& dbuf);
        inline bool encrypted() { return (ess_algid != 0x80); }
	void send_msg(const std::string msg_str, long msg_type);

  // internal instance variables and state
  int write_bufp;
  char write_buf[512];
  const char *d_udp_host;
  int  d_sys_num;
  int  d_port;
  int  d_debug;
  bool d_do_imbe;
  bool d_do_output;
  bool d_do_msgq;
	bool d_do_audio_output;
  bool d_do_nocrypt;
  Rx_Status rx_status;
  double error_history[20];
  gr::msg_queue::sptr  d_msg_queue;
  std::deque<int16_t>& output_queue;
  p25_framer *framer;
  long curr_src_id;
  struct timeval last_qtime;
  p25p1_voice_decode p1voice_decode;
        const op25_audio& op25audio;
	log_ts logts;

        ezpwd::RS<63,55> rs8;  // Reed-Solomon decoders for 8, 12 and 16 bit parity
        ezpwd::RS<63,51> rs12;
        ezpwd::RS<63,47> rs16;

        uint16_t  ess_keyid;
        uint8_t ess_algid;
	uint8_t  ess_mi[9] = {0};
	uint16_t vf_tgid;

     public:
  void reset_timer();
	void rx_sym (const uint8_t *syms, int nsyms);
        p25p1_fdma(int sys_num, const op25_audio& udp, int debug, bool do_imbe, bool do_output, bool do_msgq, gr::msg_queue::sptr queue, std::deque<int16_t> &output_queue, bool do_audio_output, bool do_nocrypt);
       ~p25p1_fdma();

  // Where all the action really happens
  long get_curr_src_id();
  void reset_rx_status();
  Rx_Status get_rx_status();
  void clear();
  int general_work(int                        noutput_items,
                   gr_vector_int            & ninput_items,
                   gr_vector_const_void_star& input_items,
                   gr_vector_void_star      & output_items);
};
} // namespace
} // namespace
#endif /* INCLUDED_OP25_REPEATER_P25P1_FDMA_H  */
