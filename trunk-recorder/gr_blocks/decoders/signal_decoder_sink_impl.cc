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

#include "signal_decoder_sink_impl.h"
#include "signal_decoder_sink.h"
#include <boost/math/special_functions/round.hpp>
#include <climits>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/thread/thread.h>
#include <stdexcept>
#include <stdio.h>

#include "fsync_decode.h"
#include "mdc_decode.h"
#include "star_decode.h"

namespace gr {
namespace blocks {

SignalType get_mdc_signal_type(unsigned char op, unsigned char arg) {
  switch (op) {
  case 0x00:
    return arg == 0x80 ? SignalType::EmergencyPre : SignalType::Emergency;
  case 0x01:
    return arg == 0x80 ? SignalType::NormalPre : SignalType::Normal;
  case 0x20:
    return SignalType::EmergencyAck;
  case 0x63:
    return SignalType::RadioCheck;
  case 0x03:
    return SignalType::RadioCheckAck;
  case 0x07:
    return SignalType::Normal; // Message
  case 0x23:
    return SignalType::Normal; // Message Ack
  case 0x22:
    return SignalType::RadioCheck;
  case 0x06:
    return SignalType::RadioCheckAck;
  case 0x11:
    return SignalType::Normal; // Remote Monitor
  case 0x2b:
    return (arg == 0x02) ? SignalType::RadioRevive : SignalType::RadioStun;
  case 0x0b:
    return (arg == 0x02) ? SignalType::RadioReviveAck : SignalType::RadioStunAck;
  default:
    return SignalType::Normal;
  }
}
void mdc_callback(int frameCount, // 1 or 2 - if 2 then extra0-3 are valid
                  unsigned char op,
                  unsigned char arg,
                  unsigned short unitID,
                  unsigned char extra0,
                  unsigned char extra1,
                  unsigned char extra2,
                  unsigned char extra3,
                  void *context) {
  char json_buffer[2048];
  snprintf(json_buffer, sizeof(json_buffer), "{\"type\":\"MDC1200\","
                                             "\"timestamp\":\"%d\","
                                             "\"op\":\"%02x\","
                                             "\"arg\":\"%02x\","
                                             "\"unitID\":\"%04x\","
                                             "\"ex0\":\"%02x\","
                                             "\"ex1\":\"%02x\","
                                             "\"ex2\":\"%02x\","
                                             "\"ex3\":\"%02x\"}\n",
           (int)time(NULL), op, arg, unitID, extra0, extra1, extra2, extra3);

  BOOST_LOG_TRIVIAL(info) << json_buffer;

  signal_decoder_sink_impl *decoder = (signal_decoder_sink_impl *)context;

  decoder->log_decoder_msg(unitID, "MDC1200", get_mdc_signal_type(op, arg));
}

void fsync_callback(int cmd, int subcmd, int from_fleet, int from_unit, int to_fleet, int to_unit, int allflag, unsigned char *payload, int payload_len, unsigned char *raw_msg, int raw_msg_len, void *context, int is_fsync2, int is_2400) {
  char json_buffer[2048];
  snprintf(json_buffer, sizeof(json_buffer), "{\"type\":\"FLEETSYNC\","
                                             "\"timestamp\":\"%d\","
                                             "\"cmd\":\"%d\","
                                             "\"subcmd\":\"%d\","
                                             "\"from_fleet\":\"%d\","
                                             "\"from_unit\":\"%d\","
                                             "\"to_fleet\":\"%d\","
                                             "\"to_unit\":\"%d\","
                                             "\"all_flag\":\"%d\","
                                             "\"payload\":\"%.*s\","
                                             "\"fsync2\":\"%d\","
                                             "\"2400\":\"%d\"}\n",
           (int)time(NULL), cmd, subcmd, from_fleet, from_unit,
           to_fleet, to_unit, allflag,
           payload_len, payload,
           is_fsync2, is_2400);

  BOOST_LOG_TRIVIAL(info) << json_buffer;

  signal_decoder_sink_impl *decoder = (signal_decoder_sink_impl *)context;
  decoder->log_decoder_msg(from_unit, "FLEETSYNC", SignalType::Normal);
}

void star_callback(int unitID, int tag, int status, int message, void *context) {
  char json_buffer[2048];
  snprintf(json_buffer, sizeof(json_buffer), "{\"type\":\"STAR\","
                                             "\"timestamp\":\"%d\","
                                             "\"unitID\":\"%d\","
                                             "\"tag\":\"%d\","
                                             "\"status\":\"%d\","
                                             "\"message\":\"%d\"}\n",
           (int)time(NULL), unitID, tag, status, message);

  BOOST_LOG_TRIVIAL(info) << json_buffer;

  signal_decoder_sink_impl *decoder = (signal_decoder_sink_impl *)context;
  decoder->log_decoder_msg(unitID, "STAR", SignalType::Normal);
}

signal_decoder_sink_impl::sptr
signal_decoder_sink_impl::make(unsigned int sample_rate, decoder_callback callback) {
  return gnuradio::get_initial_sptr(new signal_decoder_sink_impl(sample_rate, callback));
}

signal_decoder_sink_impl::signal_decoder_sink_impl(unsigned int sample_rate, decoder_callback callback)
    : sync_block("signal_decoder_sink_impl",
                 io_signature::make(1, 1, sizeof(float)),
                 io_signature::make(0, 0, 0)),
      d_callback(callback),
      d_mdc_enabled(false),
      d_fsync_enabled(false),
      d_star_enabled(false) {
  d_mdc_decoder = mdc_decoder_new(sample_rate);
  d_fsync_decoder = fsync_decoder_new(sample_rate);
  d_star_decoder = star_decoder_new(sample_rate);

  mdc_decoder_set_callback(d_mdc_decoder, mdc_callback, this);
  fsync_decoder_set_callback(d_fsync_decoder, fsync_callback, this);
  star_decoder_set_callback(d_star_decoder, star_format_1_16383, star_callback, this);
}

void signal_decoder_sink_impl::set_mdc_enabled(bool b) { d_mdc_enabled = b; };
void signal_decoder_sink_impl::set_fsync_enabled(bool b) { d_fsync_enabled = b; };
void signal_decoder_sink_impl::set_star_enabled(bool b) { d_star_enabled = b; };

bool signal_decoder_sink_impl::get_mdc_enabled() { return d_mdc_enabled; };
bool signal_decoder_sink_impl::get_fsync_enabled() { return d_fsync_enabled; };
bool signal_decoder_sink_impl::get_star_enabled() { return d_star_enabled; };

int signal_decoder_sink_impl::work(int noutput_items, gr_vector_const_void_star &input_items, gr_vector_void_star &output_items) {

  gr::thread::scoped_lock guard(d_mutex); // hold mutex for duration of this

  return dowork(noutput_items, input_items, output_items);
}

int signal_decoder_sink_impl::dowork(int noutput_items, gr_vector_const_void_star &input_items, gr_vector_void_star &output_items) {

  if (d_mdc_enabled) {
    mdc_decoder_process_samples(d_mdc_decoder, (float *)input_items[0], noutput_items);
  }

  if (d_fsync_enabled) {
    fsync_decoder_process_samples(d_fsync_decoder, (float *)input_items[0], noutput_items);
  }

  if (d_star_enabled) {
    star_decoder_process_samples(d_star_decoder, (float *)input_items[0], noutput_items);
  }

  return noutput_items;
}

void signal_decoder_sink_impl::log_decoder_msg(long unitId, const char *signaling_type, SignalType signal) {
  if (d_callback != NULL) {
    d_callback(unitId, signaling_type, signal);
  } else {
    BOOST_LOG_TRIVIAL(warning) << "log_decoder_msg dropped, no callback setup!";
  }
}
} /* namespace blocks */
} /* namespace gr */
