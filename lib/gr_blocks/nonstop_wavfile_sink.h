/* -*- c++ -*- */
/*
 * Copyright 2008,2009,2013 Free Software Foundation, Inc.
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

#ifndef INCLUDED_GR_NONSTOP_WAVFILE_SINK_H
#define INCLUDED_GR_NONSTOP_WAVFILE_SINK_H

#include "../trunk-recorder/call.h"
#include "../trunk-recorder/call_conventional.h"
#include <boost/log/trivial.hpp>
#include <gnuradio/blocks/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace blocks {

/*!
 * \brief Write stream to a Microsoft PCM (.wav) file.
 * \ingroup audio_blk
 *
 * \details
 * Values must be floats within [-1;1].
 * Check gr_make_wavfile_sink() for extra info.
 */
class BLOCKS_API nonstop_wavfile_sink : virtual public sync_block
{
public:
	// gr::blocks::wavfile_sink::sptr
	typedef boost::shared_ptr<nonstop_wavfile_sink> sptr;

	/*!
	 * \brief Opens a new file and writes a WAV header. Thread-safe.
	 */
	virtual bool open(const char* filename) = 0;

	/*!
	 * \brief Closes the currently active file and completes the WAV
	 * header. Thread-safe.
	 */
	virtual void close() = 0;

	/*!
	 * \brief Set the sample rate. This will not affect the WAV file
	 * currently opened. Any following open() calls will use this new
	 * sample rate.
	 */
	virtual void set_sample_rate(unsigned int sample_rate) = 0;

	/*!
	 * \brief Set bits per sample. This will not affect the WAV file
	 * currently opened (see set_sample_rate()). If the value is
	 * neither 8 nor 16, the call is ignored and the current value
	 * is kept.
	 */
	virtual void set_bits_per_sample(int bits_per_sample) = 0;
	virtual double length_in_seconds() = 0;
};

} /* namespace blocks */
} /* namespace gr */

#endif /* INCLUDED_GR_WAVFILE_SINK_H */
