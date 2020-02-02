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

#include "mp3_file_sink_impl.h"
#include "mp3_file_sink.h"
#include <gnuradio/io_signature.h>
#include <stdexcept>
#include <climits>
#include <cstring>
#include <cmath>
#include <fcntl.h>
#include <gnuradio/thread/thread.h>
#include <boost/math/special_functions/round.hpp>
#include <stdio.h>

 // win32 (mingw/msvc) specific
#ifdef HAVE_IO_H
# include <io.h>
#endif // ifdef HAVE_IO_H
#ifdef O_BINARY
# define OUR_O_BINARY O_BINARY
#else // ifdef O_BINARY
# define OUR_O_BINARY 0
#endif // ifdef O_BINARY

// should be handled via configure
#ifdef O_LARGEFILE
# define OUR_O_LARGEFILE O_LARGEFILE
#else // ifdef O_LARGEFILE
# define OUR_O_LARGEFILE 0
#endif // ifdef O_LARGEFILE

namespace gr {
    namespace blocks {
        mp3_file_sink_impl::sptr
            mp3_file_sink_impl::make(int          n_channels,
                unsigned int sample_rate,
                int          bits_per_sample)
        {
            return gnuradio::get_initial_sptr
            (new mp3_file_sink_impl(n_channels,
                sample_rate, bits_per_sample));
        }

        mp3_file_sink_impl::mp3_file_sink_impl(
            int          n_channels,
            unsigned int sample_rate,
            int          bits_per_sample)
            : sync_block("mp3_file_sink",
                io_signature::make(1, n_channels, sizeof(float)),
                io_signature::make(0, 0, 0)),
            d_sample_rate(sample_rate), d_nchans(n_channels),
           d_fp(0)
        {
            if ((bits_per_sample != 8) && (bits_per_sample != 16)) {
                throw std::runtime_error("Invalid bits per sample (supports 8 and 16)");
            }
            d_bytes_per_sample = bits_per_sample / 8;
        }

        char* mp3_file_sink_impl::get_filename() {
            return current_filename;
        }

        bool mp3_file_sink_impl::open(const char* filename) {
            gr::thread::scoped_lock guard(d_mutex);
            return open_internal(filename);
        }

        bool mp3_file_sink_impl::open_internal(const char* filename) {
            
            bool bExists = boost::filesystem::exists(filename);

            // we use the open system call to get access to the O_LARGEFILE flag.
            //  O_APPEND|
            int fd;

            if ((fd = ::open(filename,
                O_RDWR | O_CREAT | OUR_O_LARGEFILE | OUR_O_BINARY,
                0664)) < 0) {
                perror(filename);
                BOOST_LOG_TRIVIAL(error) << "mp3 error opening: " << filename << std::endl;
                return false;
            }

            if (d_fp) { // if we've already got a new one open, close it
                BOOST_LOG_TRIVIAL(trace) << "File pointer already open, closing " << d_fp << " more" << current_filename << " for " << filename << std::endl;

                // fclose(d_fp);
                // d_fp = NULL;
            }

            if (strlen(filename) >= 255) {
                BOOST_LOG_TRIVIAL(error) << "mp3_file_sink: Error! filename longer than 255";
            }
            else {
                strcpy(current_filename, filename);
            }

            if ((d_fp = fdopen(fd, "rb+")) == NULL) {
                perror(filename);
                ::close(fd); // don't leak file descriptor if fdopen fails.
                BOOST_LOG_TRIVIAL(error) << "mp3 open failed" << std::endl;
                return false;
            }

            d_lame = lame_init();
            if (!d_lame) {
                BOOST_LOG_TRIVIAL(error) << "lame_init failed!" << std::endl;
                return false;
            }

            lame_set_in_samplerate(d_lame, d_sample_rate);
            lame_set_VBR(d_lame, vbr_mtrh);
            lame_set_brate(d_lame, d_bytes_per_sample * 8);
            lame_set_quality(d_lame, 7);
            //lowpass * highpass lame_set_lowpassfreq lame_set_highpassfreq
            lame_set_out_samplerate(d_lame, d_sample_rate);
            lame_set_num_channels(d_lame, d_nchans);
            lame_set_mode(d_lame, d_nchans == 1 ? MONO: STEREO);
            lame_init_params(d_lame);

            d_sample_count = 0;

            if (bExists) {
                fseek(d_fp, 0, SEEK_END);
            }
            else {
                // you have to rewind the d_new_fp because the read failed.
                if (fseek(d_fp, 0, SEEK_SET) != 0) {
                    BOOST_LOG_TRIVIAL(error) << "Error rewinding " << std::endl;
                    return false;
                }
            }

            if (d_bytes_per_sample == 1) {
                d_max_sample_val = UCHAR_MAX;
                d_min_sample_val = 0;
                d_normalize_fac = d_max_sample_val / 2;
                d_normalize_shift = 1;
            }
            else if (d_bytes_per_sample == 2) {
                d_max_sample_val = SHRT_MAX;
                d_min_sample_val = SHRT_MIN;
                d_normalize_fac = d_max_sample_val;
                d_normalize_shift = 0;
            }

            return true;
        }

        void
            mp3_file_sink_impl::close()
        {
            gr::thread::scoped_lock guard(d_mutex);
            
            if (!d_fp) {
                BOOST_LOG_TRIVIAL(error) << "mp3 error closing file" << std::endl;
                return;
            }

            close_mp3();
        }

        void
            mp3_file_sink_impl::close_mp3()
        {

            int outBytes = lame_encode_flush(d_lame, d_lamebuf, LAMEBUF_SIZE);
            fwrite(d_lamebuf, sizeof(char), outBytes, d_fp);

            fclose(d_fp);
            d_fp = NULL;

            // std::cout <<  "Closing mp3 File - byte count: " << byte_count << " samples:
            // " << d_sample_count << " bytes per: " << d_bytes_per_sample << std::endl;
        }

        mp3_file_sink_impl::~mp3_file_sink_impl()
        {
            close();
        }

        bool mp3_file_sink_impl::stop()
        {
            return true;
        }

        int mp3_file_sink_impl::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) {

            gr::thread::scoped_lock guard(d_mutex); // hold mutex for duration of this

            return dowork(noutput_items, input_items, output_items);
        }

        int mp3_file_sink_impl::dowork(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) {
            // block

            int n_in_chans = input_items.size();

            int nwritten;
            int mp3_bytes;

            if (!d_fp) // drop output on the floor
            {
                BOOST_LOG_TRIVIAL(error) << "MP3 - Dropping items, no fp: " << noutput_items << " Filename: " << current_filename << " Current sample count: " << d_sample_count << std::endl;
                return noutput_items;
            }

            std::vector<gr::tag_t> tags;
            pmt::pmt_t this_key(pmt::intern("src_id"));
            get_tags_in_range(tags, 0, nitems_read(0), nitems_read(0) + noutput_items);

            // std::cout << std::endl;

            if (noutput_items <= 0) {
                return noutput_items;
            }
            else {
                BOOST_LOG_TRIVIAL(info) << "MP3 - Writing " << noutput_items << " bytes to " << current_filename << std::endl;
            }

            if (n_in_chans == 1)
            {
                mp3_bytes = lame_encode_buffer_ieee_float(d_lame, (const float*)&input_items[0], NULL, noutput_items, d_lamebuf, LAMEBUF_SIZE);
            }
            else
            {
                mp3_bytes = lame_encode_buffer_ieee_float(d_lame, (const float*)&input_items[0], (const float*)&input_items[1], noutput_items, d_lamebuf, LAMEBUF_SIZE);
            }
            nwritten = fwrite(d_lamebuf, sizeof(unsigned char), mp3_bytes, d_fp);

            if (feof(d_fp) || ferror(d_fp) || nwritten != mp3_bytes) {
                fprintf(stderr, "[%s] file i/o error\n", __FILE__);
                close();
                return nwritten;
            }
            d_sample_count+= noutput_items;

            // fflush (d_fp);  // this is added so unbuffered content is written.
            tags.clear();
            return nwritten;
        }

        double
            mp3_file_sink_impl::length_in_seconds()
        {
            // std::cout << "Filename: "<< current_filename << "Sample #: " <<
            // d_sample_count << " rate: " << d_sample_rate << " bytes: " <<
            // d_bytes_per_sample << "\n";
            return (double)d_sample_count / (double)d_sample_rate;

            // return (double) ( d_sample_count * d_bytes_per_sample_new * 8) / (double)
            // d_sample_rate;
        }

        void
            mp3_file_sink_impl::do_update()
        {}
    } /* namespace blocks */
} /* namespace gr */
