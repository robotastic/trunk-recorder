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

#ifndef INCLUDE_GR_MP3_FILE_SINK_H
#define INCLUDE_GR_MP3_FILE_SINK_H

#include <string>
#include <gnuradio/sync_block.h>
#include <gnuradio/blocks/float_to_short.h>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <lame/lame.h>
#include "recording_file_sink.h"

namespace gr {
	namespace blocks {

		class BLOCKS_API mp3_file_sink : virtual public recording_file_sink
		{
		public:
			virtual bool open(const char* filename) = 0;
			virtual void close() = 0;
			virtual double length_in_seconds() = 0;
		};

	} /* namespace blocks */
} /* namespace gr */

#endif /* INCLUDE_GR_MP3_FILE_SINK_H */