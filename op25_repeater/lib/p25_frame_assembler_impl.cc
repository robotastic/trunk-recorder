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
	static const char wbuf[2] = {0xff, 0xff}; // dummy NAC
	if (!d_do_msgq)
		return;
	if (d_msg_queue->full_p())
		return;
	gr::message::sptr msg = gr::message::make_from_string(std::string(wbuf, 2), duid, 0, 0);
	d_msg_queue->insert_tail(msg);
    }

    void p25_frame_assembler_impl::set_xormask(const char*p) {
	p2tdma.set_xormask(p);
    }

    void p25_frame_assembler_impl::set_slotid(int slotid) {
	p2tdma.set_slotid(slotid);
    }

    p25_frame_assembler::sptr
    p25_frame_assembler::make(int sys_id, const char* udp_host, int port, int debug, bool do_imbe, bool do_output, bool do_msgq, gr::msg_queue::sptr queue, bool do_audio_output, bool do_phase2_tdma)
    {
      return gnuradio::get_initial_sptr
        (new p25_frame_assembler_impl(sys_id, udp_host, port, debug, do_imbe, do_output, do_msgq, queue, do_audio_output, do_phase2_tdma));
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

static const int MIN_IN = 1;	// mininum number of input streams
static const int MAX_IN = 1;	// maximum number of input streams

/*
 * The private constructor
 */
    p25_frame_assembler_impl::p25_frame_assembler_impl(int sys_id, const char* udp_host, int port, int debug, bool do_imbe, bool do_output, bool do_msgq, gr::msg_queue::sptr queue, bool do_audio_output, bool do_phase2_tdma)
      : gr::block("p25_frame_assembler",
		   gr::io_signature::make (MIN_IN, MAX_IN, sizeof (char)),
		   gr::io_signature::make ((do_output || do_audio_output) ? 1 : 0, (do_output || do_audio_output) ? 1 : 0, (do_audio_output) ? sizeof(int16_t) : ((do_output) ? sizeof(char) : 0 ))),
	d_do_imbe(do_imbe),
	d_do_output(do_output),
	output_queue(),
	p1fdma(sys_id, udp_host, port, debug, do_imbe, do_output, do_msgq, queue, output_queue, do_audio_output),
	d_do_audio_output(do_audio_output),
	d_do_phase2_tdma(do_phase2_tdma),
	p2tdma(0, debug, output_queue),
	d_do_msgq(do_msgq),
	d_msg_queue(queue)
{
	if (d_do_audio_output && !d_do_output)
		fprintf(stderr, "p25_frame_assembler: error: do_output must be enabled if do_audio_output is enabled\n");
	if (d_do_audio_output && !d_do_imbe)
		fprintf(stderr, "p25_frame_assembler: error: do_imbe must be enabled if do_audio_output is enabled\n");
	if (d_do_phase2_tdma && !d_do_audio_output)
		fprintf(stderr, "p25_frame_assembler: error: do_audio_output must be enabled if do_phase2_tdma is enabled\n");
}

void
p25_frame_assembler_impl::forecast(int nof_output_items, gr_vector_int &nof_input_items_reqd)
{
   // for do_imbe=false: we output packed bytes (4:1 ratio)
   // for do_imbe=true: input rate= 4800, output rate= 1600 = 32 * 50 (3:1)
   // for do_audio_output: output rate=8000 (ratio 0.6:1)
   const size_t nof_inputs = nof_input_items_reqd.size();
   double samples_reqd = 4.0 * nof_output_items;
    int nof_samples_reqd;
   if (d_do_imbe)
     samples_reqd = 3.0 * nof_output_items;
   samples_reqd = nof_output_items;
   if (d_do_audio_output)
     samples_reqd = 0.6 * nof_output_items;

   nof_samples_reqd = (int)ceil(samples_reqd);
    for(int i = 0; i < nof_inputs; i++) {
      nof_input_items_reqd[i] = nof_samples_reqd;
    }
}


int
p25_frame_assembler_impl::general_work (int noutput_items,
                               gr_vector_int &ninput_items,
                               gr_vector_const_void_star &input_items,
                               gr_vector_void_star &output_items)
{

  const uint8_t *in = (const uint8_t *) input_items[0];

  p1fdma.rx_sym(in, ninput_items[0]);
  if(d_do_phase2_tdma) {
	for (int i = 0; i < ninput_items[0]; i++) {
		if(p2tdma.rx_sym(in[i])) {
			int rc = p2tdma.handle_frame();
			if (rc > -1)
				p25p2_queue_msg(rc);
		}
	}
  }
  int amt_produce = 0;
  if (d_do_output) {
    amt_produce = noutput_items;
    if (amt_produce > (int)output_queue.size())
      amt_produce = output_queue.size();
    if (amt_produce > 0) {
      if (d_do_audio_output) {
        int16_t *out = (int16_t *) output_items[0];
        for (int i=0; i < amt_produce; i++) {
          out[i] = output_queue[i];
        }
      } else {
        unsigned char *out = (unsigned char *) output_items[0];
        for (int i=0; i < amt_produce; i++) {
          out[i] = output_queue[i];
        }
      }
      output_queue.erase(output_queue.begin(), output_queue.begin() + amt_produce);
    }
  }
  consume_each(ninput_items[0]);
  // Tell runtime system how many output items we produced.
  return amt_produce;
}

  } /* namespace op25_repeater */
} /* namespace gr */
