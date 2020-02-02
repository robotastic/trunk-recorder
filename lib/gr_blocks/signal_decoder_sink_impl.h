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

#ifndef INCLUDED_GR_SIGNAL_DECODER_SINK_IMPL_H
#define INCLUDED_GR_SIGNAL_DECODER_SINK_IMPL_H

#include "signal_decoder_sink.h"
#include <boost/log/trivial.hpp>

#include "mdc_decode.h"
#include "fsync_decode.h"
#include "star_decode.h"

namespace gr {
	namespace blocks {

		class signal_decoder_sink_impl : public signal_decoder_sink
		{
		private:
			mdc_decoder_t * d_mdc_decoder;
			fsync_decoder_t * d_fsync_decoder;
			star_decoder_t * d_star_decoder;
			Call * d_current_call;

			bool d_mdc_enabled;
			bool d_fsync_enabled;
			bool d_star_enabled;

		protected:

			boost::mutex d_mutex;
			virtual int dowork(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items);

		public:

			typedef boost::shared_ptr <signal_decoder_sink_impl> sptr;

			/*
			 * \param filename The .wav file to be opened
			 * \param n_channels Number of channels (2 = stereo or I/Q output)
			 * \param sample_rate Sample rate [S/s]
			 * \param bits_per_sample 16 or 8 bit, default is 16
			 */
			static sptr make(unsigned int sample_rate);

			signal_decoder_sink_impl(unsigned int sample_rate);

			virtual int work(int noutput_items,
				gr_vector_const_void_star& input_items,
				gr_vector_void_star& output_items);

			void set_mdc_enabled(bool b);
			void set_fsync_enabled(bool b);
			void set_star_enabled(bool b);

			bool get_mdc_enabled();
			bool get_fsync_enabled();
			bool get_star_enabled();

			void set_call(Call* call);
			void end_call();

			void log_decoder_msg(std::string msg);
		};

	} /* namespace blocks */
} /* namespace gr */

#endif /* INCLUDED_GR_SIGNAL_DECODER_SINK_IMPL_H */
