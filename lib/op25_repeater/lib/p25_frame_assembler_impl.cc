/* -*- c++ -*- */
/*
 * Copyright 2010, 2011, 2012, 2013, 2014 Max H. Parke KA1RBI
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "p25_frame_assembler_impl.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <sys/time.h>

namespace gr {
namespace op25_repeater {

void p25_frame_assembler_impl::p25p2_queue_msg(int duid)
{
	static const unsigned char wbuf[2] = {0xff, 0xff}; // dummy NAC
	if (!d_do_msgq)
		return;
	if (d_msg_queue->full_p())
		return;
	gr::message::sptr msg = gr::message::make_from_string(std::string((const char *)wbuf, 2), duid, d_sys_num, 0);
  d_msg_queue->insert_tail(msg);
}

void p25_frame_assembler_impl::set_xormask(const char *p) {
  p2tdma.set_xormask(p);
}

void p25_frame_assembler_impl::set_slotid(int slotid) {
  p2tdma.set_slotid(slotid);
}

p25_frame_assembler::sptr
    p25_frame_assembler::make(int sys_num, int silence_frames, const char* udp_host, int port, int debug, bool do_imbe, bool do_output, bool do_msgq, gr::msg_queue::sptr queue, bool do_audio_output, bool do_phase2_tdma, bool do_nocrypt)
{
  return gnuradio::get_initial_sptr(new p25_frame_assembler_impl(sys_num, silence_frames, udp_host, port, debug, do_imbe, do_output, do_msgq, queue, do_audio_output, do_phase2_tdma, do_nocrypt));
}

/*
 * The private constructor
 */

/*
 * Our virtual destructor.
 */
p25_frame_assembler_impl::~p25_frame_assembler_impl()
    {
    }

static const int MIN_IN = 1; // mininum number of input streams
static const int MAX_IN = 1; // maximum number of input streams

/*
 * The private constructor
 */
    p25_frame_assembler_impl::p25_frame_assembler_impl(int sys_num, int silence_frames, const char* udp_host, int port, int debug, bool do_imbe, bool do_output, bool do_msgq, gr::msg_queue::sptr queue, bool do_audio_output, bool do_phase2_tdma, bool do_nocrypt)
  : gr::block("p25_frame_assembler",
              gr::io_signature::make(MIN_IN, MAX_IN, sizeof(char)),
		   gr::io_signature::make ((do_output) ? 1 : 0, (do_output) ? 1 : 0, (do_audio_output && do_output) ? sizeof(int16_t) : ((do_output) ? sizeof(char) : 0 ))),
  d_do_imbe(do_imbe),
  d_do_output(do_output),
  d_silence_frames(silence_frames),
  output_queue(),
        op25audio(udp_host, port, debug),
	p1fdma(sys_num, op25audio, debug, do_imbe, do_output, do_msgq, queue, output_queue, do_audio_output, do_nocrypt),
  d_do_audio_output(do_audio_output),
  d_do_phase2_tdma(do_phase2_tdma),
	d_do_nocrypt(do_nocrypt),
	p2tdma(op25audio, 0, debug, do_msgq, queue, output_queue, do_audio_output, do_nocrypt),
  d_do_msgq(do_msgq),
  d_msg_queue(queue),
  d_input_rate(4800),
  d_tag_key(pmt::intern("src_id")),
  d_tag_src(pmt::intern(name()))
{
  if (d_do_audio_output && !d_do_output) fprintf(stderr, "p25_frame_assembler: error: do_output must be enabled if do_audio_output is enabled\n");

  if (d_do_audio_output && !d_do_imbe) fprintf(stderr, "p25_frame_assembler: error: do_imbe must be enabled if do_audio_output is enabled\n");

  if (d_do_phase2_tdma && !d_do_audio_output) fprintf(stderr, "p25_frame_assembler: error: do_audio_output must be enabled if do_phase2_tdma is enabled\n");

  if (d_do_audio_output) set_output_multiple(864);

  if (!d_do_audio_output && !d_do_imbe) set_output_multiple(160);
}

void
p25_frame_assembler_impl::forecast(int nof_output_items, gr_vector_int& nof_input_items_reqd)
{
  // for do_imbe=false: we output packed bytes (4:1 ratio)
  // for do_imbe=true: input rate= 4800, output rate= 1600 = 32 * 50 (3:1)
  // for do_audio_output: output rate=8000 (ratio 0.6:1)
  const size_t nof_inputs = nof_input_items_reqd.size();
  double samples_reqd     = 4.0 * nof_output_items;
  int    nof_samples_reqd;

  samples_reqd = nof_output_items;

  if (d_do_imbe) {
    samples_reqd = 3.0 * nof_output_items;
  }

  if (d_do_audio_output) {
    //samples_reqd = (int)std::ceil(float(d_input_rate / 8000) * float(nof_output_items));

    if (d_do_phase2_tdma) {
      samples_reqd = floor(0.4 * nof_output_items);
    } else {
      samples_reqd = floor(0.6 * nof_output_items);
    }
  }
  nof_samples_reqd = (int) samples_reqd;

  for (int i = 0; i < nof_inputs; i++) {
    nof_input_items_reqd[i] = nof_samples_reqd;
  }
}

void p25_frame_assembler_impl::reset_rx_status() {
  p1fdma.reset_rx_status();
}

Rx_Status p25_frame_assembler_impl::get_rx_status() {
  return p1fdma.get_rx_status();
}

void p25_frame_assembler_impl::clear() {
  p1fdma.clear();
}

int
p25_frame_assembler_impl::general_work(int                        noutput_items,
                                       gr_vector_int            & ninput_items,
                                       gr_vector_const_void_star& input_items,
                                       gr_vector_void_star      & output_items)
{

  const uint8_t *in = (const uint8_t *)input_items[0];

  p1fdma.rx_sym(in, ninput_items[0]);
  if (d_do_phase2_tdma) {
    for (int i = 0; i < ninput_items[0]; i++) {
      if (p2tdma.rx_sym(in[i])) {
        int rc = p2tdma.handle_frame();
			if (rc > -1) {
          p25p2_queue_msg(rc);
				p1fdma.reset_timer(); // prevent P1 timeouts due to long TDMA transmissions
        }
      }
    }
  }
  int amt_produce = 0;

  if (d_do_audio_output) {
    amt_produce = noutput_items;
    int16_t *out = (int16_t *)output_items[0];


    if (amt_produce > (int)output_queue.size()) {
      // BOOST_LOG_TRIVIAL(info) << "Amt Prod: " << amt_produce << " output_queue: " << output_queue.size() << "
      // noutput_items: " <<  noutput_items;
      amt_produce = output_queue.size();
    }


    if (amt_produce > 0) {
      long src_id = p1fdma.get_curr_src_id();

      if (src_id) {
        // fprintf(stderr,  "tagging source: %ld at %lu\n", src_id,
        //  nitems_written(0));
        add_item_tag(0, nitems_written(0), d_tag_key, pmt::from_long(src_id), d_tag_src);
      }

      for (int i = 0; i < amt_produce; i++) {
        out[i] = output_queue[i];
      }
      output_queue.erase(output_queue.begin(), output_queue.begin() + amt_produce);

      /*
         if (amt_produce < noutput_items) {
         std::fill(out + amt_produce, out + noutput_items, 0);
         amt_produce = noutput_items;
         }*/
      silence_frame_count = d_silence_frames;
    } else if (silence_frame_count > 0) {
      std::fill(out, out + noutput_items, 0);
      amt_produce = noutput_items;
      silence_frame_count--;
    }
  }
  consume_each(ninput_items[0]);

  // Tell runtime system how many output items we produced.
  return amt_produce;
}

void p25_frame_assembler_impl::clear_silence_frame_count() {
  silence_frame_count = 0;
}

void p25_frame_assembler_impl::set_phase2_tdma(bool p)
{
  d_do_phase2_tdma = p;


  if (d_do_audio_output) {
    if (d_do_phase2_tdma) {
      d_input_rate = 6000;
      set_output_multiple(640);
    } else {
      d_input_rate = 4800;
      set_output_multiple(864);
    }
  }
}
} /* namespace op25_repeater */
} /* namespace gr */
