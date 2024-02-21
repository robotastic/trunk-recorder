/* -*- c++ -*- */

/*
 * Copyright 2020 Free Software Foundation, Inc.
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
#endif // ifdef HAVE_CONFIG_H

#include "tps_decoder_sink.h"
#include "tps_decoder_sink_impl.h"
#include <boost/math/special_functions/round.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <climits>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/thread/thread.h>
#include <iostream>
#include <op25_repeater/lib/op25_msg_types.h>
#include <sstream>
#include <stdexcept>
#include <stdio.h>

namespace gr {
namespace blocks {

tps_decoder_sink_impl::sptr
tps_decoder_sink_impl::make(unsigned int sample_rate, decoder_callback callback) {
  return gnuradio::get_initial_sptr(new tps_decoder_sink_impl(sample_rate, callback));
}

tps_decoder_sink_impl::tps_decoder_sink_impl(unsigned int sample_rate, decoder_callback callback)
    : hier_block2("tps_decoder_sink_impl",
                  io_signature::make(1, 1, sizeof(float)),
                  io_signature::make(0, 0, 0)),
      d_callback(callback) {
  rx_queue = gr::msg_queue::make(100);

  valve = gr::blocks::copy::make(sizeof(float));
  valve->set_enabled(false);

  initialize_p25();
}

std::string tps_decoder_sink_impl::to_hex(const std::string &s, bool upper, bool spaced) {
  std::ostringstream result;

  unsigned int c;
  for (std::string::size_type i = 0; i < s.length(); i++) {
    if (spaced && i > 0)
      result << " ";

    c = (unsigned int)(unsigned char)s[i];
    result << std::hex << std::setfill('0') << std::setw(2) << (upper ? std::uppercase : std::nouppercase) << c;
  }

  return result.str();
}

void tps_decoder_sink_impl::parse_p25_json(std::string json) {
  try {

    if (json.empty() || json.length() < 3)
      return;

    std::stringstream ss;
    ss << json;
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(ss, pt);

    int srcaddr = atoi(pt.get<std::string>("srcaddr", "0").c_str());

    if (srcaddr > 0) {
      log_decoder_msg(srcaddr, "TPS", SignalType::Normal);
    }
  } catch (std::exception const &e) {
    BOOST_LOG_TRIVIAL(error) << "TPS: ERROR PROCESSING JSON: " << e.what();
  }
}

void tps_decoder_sink_impl::process_message(gr::message::sptr msg) {
  if (msg == 0)
    return;

  long type = msg->type();

  BOOST_LOG_TRIVIAL(trace) << "TPS MESSAGE " << std::dec << type << ": " << to_hex(msg->to_string());

  if (type == M_P25_JSON_DATA) {
    parse_p25_json(msg->to_string());
    return;
  }

  if (type < 0) {
    return;
  }

  std::string s = msg->to_string();

 if (s.length() < 2) {
    BOOST_LOG_TRIVIAL(error) << "TPS Decode error, Message: " << s << " Len: " << s.length() << " is < 2";
    //messages.push_back(message);
    return;
  }

  // # nac is always 1st two bytes
  // ac = (ord(s[0]) << 8) + ord(s[1])
  uint8_t s0 = (int)s[0];
  uint8_t s1 = (int)s[1];
  int shift = s0 << 8;
  long nac = shift + s1;

  if (nac == 0xffff) {
    // dummy message, ignore it...
    return;
  }

  if (type == 7) { // # trunk: TSBK
    boost::dynamic_bitset<> b((s.length() + 2) * 8);

    for (unsigned int i = 0; i < s.length(); ++i) {
      unsigned char c = (unsigned char)s[i];
      b <<= 8;

      for (int j = 0; j < 8; j++) {
        if (c & 0x1) {
          b[j] = 1;
        } else {
          b[j] = 0;
        }
        c >>= 1;
      }
    }
    b <<= 16; // for missing crc

    decode_tsbk(b, nac);
  } else if (type == 12) { // # trunk: MBT
    std::string s1 = s.substr(0, 10);
    std::string s2 = s.substr(10);
    boost::dynamic_bitset<> header((s1.length() + 2) * 8);

    for (unsigned int i = 0; i < s1.length(); ++i) {
      unsigned char c = (unsigned char)s1[i];
      header <<= 8;

      for (int j = 0; j < 8; j++) {
        if (c & 0x1) {
          header[j] = 1;
        } else {
          header[j] = 0;
        }
        c >>= 1;
      }
    }
    header <<= 16; // for missing crc

    boost::dynamic_bitset<> mbt_data((s2.length() + 4) * 8);
    for (unsigned int i = 0; i < s2.length(); ++i) {
      unsigned char c = (unsigned char)s2[i];
      mbt_data <<= 8;

      for (int j = 0; j < 8; j++) {
        if (c & 0x1) {
          mbt_data[j] = 1;
        } else {
          mbt_data[j] = 0;
        }
        c >>= 1;
      }
    }
    mbt_data <<= 32; // for missing crc
    unsigned long opcode = bitset_shift_mask(header, 32, 0x3f);
    unsigned long link_id = bitset_shift_mask(header, 48, 0xffffff);
    decode_mbt_data(opcode, header, mbt_data, link_id, nac);
  } else {
    // Not supported yet...
  }
}
void tps_decoder_sink_impl::process_message_queues() {
  process_message(rx_queue->delete_head_nowait());
}

void tps_decoder_sink_impl::set_enabled(bool b) { valve->set_enabled(b); };

bool tps_decoder_sink_impl::get_enabled() { return valve->enabled(); };

void tps_decoder_sink_impl::log_decoder_msg(long unitId, const char *signaling_type, SignalType signal) {
  if (d_callback != NULL) {
    d_callback(unitId, signaling_type, signal);
  }
}

void tps_decoder_sink_impl::initialize_p25() {
  // OP25 Slicer
  const float l[] = {-2.0, 0.0, 2.0, 4.0};
  std::vector<float> slices(l, l + sizeof(l) / sizeof(l[0]));
  const int msgq_id = 0;
  const int debug = 0;
  slicer = gr::op25_repeater::fsk4_slicer_fb::make(msgq_id, debug, slices);

  int udp_port = 0;
  int verbosity = 0;
  const char *wireshark_host = "127.0.0.1";
  bool do_imbe = 0;
  int silence_frames = 0;
  bool do_output = 0;
  bool do_msgq = 1;
  bool do_audio_output = 0;
  bool do_tdma = 0;
  bool do_crypt = 0;
  bool soft_vocoder = false;
  op25_frame_assembler = gr::op25_repeater::p25_frame_assembler::make(silence_frames, soft_vocoder, wireshark_host, udp_port, verbosity, do_imbe, do_output, do_msgq, rx_queue, do_audio_output, do_tdma, do_crypt);

  connect(self(), 0, valve, 0);
  connect(valve, 0, slicer, 0);
  connect(slicer, 0, op25_frame_assembler, 0);
}

unsigned long tps_decoder_sink_impl::bitset_shift_mask(boost::dynamic_bitset<> &tsbk, int shift, unsigned long long mask) {
  boost::dynamic_bitset<> bitmask(tsbk.size(), mask);
  unsigned long result = ((tsbk >> shift) & bitmask).to_ulong();

  // std::cout << "    " << std::dec<< shift << " " << tsbk.size() << " [ " <<
  // mask << " ]  = " << result << " - " << ((tsbk >> shift) & bitmask) <<
  // std::endl;
  return result;
}

void tps_decoder_sink_impl::decode_mbt_data(unsigned long opcode, boost::dynamic_bitset<> &header, boost::dynamic_bitset<> &mbt_data, unsigned long sa, unsigned long nac) {
  long unit_id = 0;
  bool emergency = false;

  BOOST_LOG_TRIVIAL(trace) << "decode_mbt_data: $" << opcode;
  if (opcode == 0x0) { // grp voice channel grant
                       // unsigned long mfrid = bitset_shift_mask(header, 72, 0xff);
    unit_id = bitset_shift_mask(header, 48, 0xffffff);
    emergency = (bool)bitset_shift_mask(header, 24, 0x80);
  } else if (opcode == 0x04) { //  Unit to Unit Voice Service Channel Grant -Extended (UU_V_CH_GRANT)
    emergency = (bool)bitset_shift_mask(header, 24, 0x80);
    unit_id = bitset_shift_mask(header, 48, 0xffffff);
  }

  if (unit_id > 0 || emergency) {
    log_decoder_msg(unit_id, "TPS", emergency ? SignalType::Emergency : SignalType::Normal);
  }
}

void tps_decoder_sink_impl::decode_tsbk(boost::dynamic_bitset<> &tsbk, unsigned long nac) {
  long unit_id = 0;
  bool emergency = false;

  unsigned long opcode = bitset_shift_mask(tsbk, 88, 0x3f); // x3f

  BOOST_LOG_TRIVIAL(trace) << "TSBK: opcode: $" << std::hex << opcode;
  if (opcode == 0x00) { // group voice chan grant
    emergency = (bool)bitset_shift_mask(tsbk, 72, 0x80);
    unit_id = bitset_shift_mask(tsbk, 16, 0xffffff);
  } else if (opcode == 0x02) { // group voice chan grant update
    unsigned long mfrid = bitset_shift_mask(tsbk, 80, 0xff);
    emergency = (bool)bitset_shift_mask(tsbk, 72, 0x80);

    if (mfrid == 0x90) {
      unit_id = bitset_shift_mask(tsbk, 16, 0xffffff);
    }
  } else if (opcode == 0x04) { //  Unit to Unit Voice Service Channel Grant (UU_V_CH_GRANT)
    emergency = (bool)bitset_shift_mask(tsbk, 72, 0x80);
    unit_id = bitset_shift_mask(tsbk, 16, 0xffffff);
  } else if (opcode == 0x06) { //  Unit to Unit Voice Channel Grant Update (UU_V_CH_GRANT_UPDT)
    emergency = (bool)bitset_shift_mask(tsbk, 72, 0x80);
    unit_id = bitset_shift_mask(tsbk, 16, 0xffffff);
  } else if (opcode == 0x20) { // Acknowledge response
    unit_id = bitset_shift_mask(tsbk, 16, 0xffffff);
  } else if (opcode == 0x28) { // Unit Group Affiliation Response
    unit_id = bitset_shift_mask(tsbk, 16, 0xffffff);
  } else if (opcode == 0x2c) { // Unit Registration Response
    // unsigned long sa = bitset_shift_mask(tsbk, 16, 0xffffff);
    unit_id = bitset_shift_mask(tsbk, 40, 0xffffff);
  } else if (opcode == 0x2f) { // Unit DeRegistration Ack
    unit_id = bitset_shift_mask(tsbk, 16, 0xffffff);
  }

  if (unit_id > 0 || emergency) {
    log_decoder_msg(unit_id, "TPS", emergency ? SignalType::Emergency : SignalType::Normal);
  }
}
} /* namespace blocks */
} /* namespace gr */
