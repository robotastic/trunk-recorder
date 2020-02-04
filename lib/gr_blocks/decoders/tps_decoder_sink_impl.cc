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
# include "config.h"
#endif // ifdef HAVE_CONFIG_H

#include "tps_decoder_sink.h"
#include "tps_decoder_sink_impl.h"
#include <gnuradio/io_signature.h>
#include <stdexcept>
#include <climits>
#include <cstring>
#include <cmath>
#include <fcntl.h>
#include <gnuradio/thread/thread.h>
#include <boost/math/special_functions/round.hpp>
#include <stdio.h>
#include <op25_repeater/lib/op25_msg_types.h>

namespace gr {
    namespace blocks {

        tps_decoder_sink_impl::sptr
            tps_decoder_sink_impl::make(unsigned int sample_rate, int src_num)
        {
            return gnuradio::get_initial_sptr
            (new tps_decoder_sink_impl(sample_rate, src_num));
        }

        tps_decoder_sink_impl::tps_decoder_sink_impl(unsigned int sample_rate, int src_num)
            : hier_block2("tps_decoder_sink_impl",
                io_signature::make(1, 1, sizeof(float)),
                io_signature::make(0, 0, 0)),
            d_src_num(src_num)
        {
            rx_queue = gr::msg_queue::make(100);

            valve = gr::blocks::copy::make(sizeof(float));
            valve->set_enabled(false);

            initialize_p25();
        }

        void tps_decoder_sink_impl::process_message(gr::message::sptr msg)
        {
            if (msg == 0) return;

            long type = msg->type();
            int src_num = msg->arg1();
            
            switch (type)
            {
            case M_P25_TIMEOUT:
            {
                BOOST_LOG_TRIVIAL(trace) << "[" << std::dec << src_num << "] TPS: P25 TIMEOUT: " << msg->to_string();
                return;
            }
            case M_P25_UI_REQ:
            {
                BOOST_LOG_TRIVIAL(trace) << "[" << std::dec << src_num << "] TPS: P25 UI_REQ: " << msg->to_string();
                return;
            }
            case M_P25_JSON_DATA:
            {
                BOOST_LOG_TRIVIAL(trace) << "[" << std::dec << src_num << "] TPS: P25 JSON_DATA: " << msg->to_string();
                return;
            }
            }

            BOOST_LOG_TRIVIAL(trace) << "[" << std::dec << src_num << "] TPS: UNKNOWN: " << std::dec << type << " : " << msg->to_string();

            if (type < 0) {    
                return;
            }

            std::string s = msg->to_string();

            // # nac is always 1st two bytes
            //ac = (ord(s[0]) << 8) + ord(s[1])
            uint8_t s0 = (int)s[0];
            uint8_t s1 = (int)s[1];
            int shift = s0 << 8;
            long nac = shift + s1;

            BOOST_LOG_TRIVIAL(trace) << "[" << std::dec << src_num << "] TPS: dec: " << std::dec << "nac " << nac << " type " << type << " size " << msg->to_string().length() << " mesg len: " << msg->length() << std::endl;
            BOOST_LOG_TRIVIAL(trace) << "[" << std::dec << src_num << "] TPS: hex: " << std::hex << "nac " << nac << " type " << type << " size " << msg->to_string().length() << " mesg len: " << msg->length() << std::endl;
        }
        void tps_decoder_sink_impl::process_message_queues()
        {
            process_message(rx_queue->delete_head_nowait());
        }

        void tps_decoder_sink_impl::set_enabled(bool b) { valve->set_enabled(b); };

        bool tps_decoder_sink_impl::get_enabled() { return valve->enabled(); };

        void tps_decoder_sink_impl::log_decoder_msg(long unitId, const char* system_type, bool emergency)
        {
            if (d_current_call == NULL)
            {
                BOOST_LOG_TRIVIAL(error) << "Unable to log: " << system_type << " : " << unitId << ", no current call.";
            }
            else
            {
                BOOST_LOG_TRIVIAL(error) << "Logging " << system_type << " : " << unitId << " to current call.";
                d_current_call->add_signal_source(unitId, system_type, emergency);
            }
        }

        void tps_decoder_sink_impl::set_call(Call* call) {
            d_current_call = call;

            if (d_current_call == NULL) {
                set_enabled(false);
            }
            else {
                set_enabled(d_current_call->get_system()->get_tps_enabled());
            }
        }
        void tps_decoder_sink_impl::end_call() {
            set_call(NULL);
        }

        void tps_decoder_sink_impl::initialize_p25()
        {
            //OP25 Slicer
            const float l[] = { -2.0, 0.0, 2.0, 4.0 };
            std::vector<float> slices(l, l + sizeof(l) / sizeof(l[0]));
            slicer = gr::op25_repeater::fsk4_slicer_fb::make(slices);
            
            int udp_port = 0;
            int verbosity = 0;
            const char* wireshark_host = "127.0.0.1";
            bool do_imbe = 0;
            bool idle_silence = 0;
            bool do_output = 0;
            bool do_msgq = 1;
            bool do_audio_output = 0;
            bool do_tdma = 0;
            bool do_crypt = 0;
            op25_frame_assembler = gr::op25_repeater::p25_frame_assembler::make(d_src_num, idle_silence, wireshark_host, udp_port, verbosity, do_imbe, do_output, do_msgq, rx_queue, do_audio_output, do_tdma, do_crypt);

            connect(self(), 0, valve, 0);
            connect(valve, 0, slicer, 0);
            connect(slicer, 0, op25_frame_assembler, 0);
        }
    } /* namespace blocks */
} /* namespace gr */
