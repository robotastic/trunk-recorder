/* -*- c++ -*- */
/*
 * Copyright 2004,2006-2011,2013 Free Software Foundation, Inc.
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
#endif

#include "nonstop_wavfile_sink_impl.h"
#include "nonstop_wavfile_sink.h"
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
#include <io.h>
#endif
#ifdef O_BINARY
#define OUR_O_BINARY O_BINARY
#else
#define OUR_O_BINARY 0
#endif

// should be handled via configure
#ifdef O_LARGEFILE
#define OUR_O_LARGEFILE O_LARGEFILE
#else
#define OUR_O_LARGEFILE 0
#endif

namespace gr {
namespace blocks {

nonstop_wavfile_sink::sptr
nonstop_wavfile_sink::make(const char *filename,
                           int n_channels,
                           unsigned int sample_rate,
                           int bits_per_sample)
{
	return gnuradio::get_initial_sptr
	       (new nonstop_wavfile_sink_impl(filename, n_channels,
	                                      sample_rate, bits_per_sample));
}

nonstop_wavfile_sink_impl::nonstop_wavfile_sink_impl(const char *filename,
        int n_channels,
        unsigned int sample_rate,
        int bits_per_sample)
	: sync_block("nonstop_wavfile_sink",
	             io_signature::make(1, n_channels, sizeof(float)),
	             io_signature::make(0, 0, 0)),
	  d_sample_rate(sample_rate), d_nchans(n_channels),
	  d_fp(0), d_new_fp(0), d_updated(false)
{
	if(bits_per_sample != 8 && bits_per_sample != 16) {
		throw std::runtime_error("Invalid bits per sample (supports 8 and 16)");
	}
	d_bytes_per_sample = bits_per_sample / 8;
	d_bytes_per_sample_new = d_bytes_per_sample;

	if(!open(filename)) {
		throw std::runtime_error("can't open file");
	}

  strcpy(old_filename, filename);
	if(bits_per_sample == 8) {
		d_max_sample_val = 0xFF;
		d_min_sample_val = 0;
		d_normalize_fac  = d_max_sample_val/2;
		d_normalize_shift = 1;
	}
	else {
		d_max_sample_val = 0x7FFF;
		d_min_sample_val = -0x7FFF;
		d_normalize_fac  = d_max_sample_val;
		d_normalize_shift = 0;
		if(bits_per_sample != 16) {
			fprintf(stderr, "Invalid bits per sample value requested, using 16");
		}
	}
}
char * nonstop_wavfile_sink_impl::get_filename(){
  return old_filename;
}
bool
nonstop_wavfile_sink_impl::open(const char* filename)
{
    int d_first_sample_pos;
    unsigned d_samples_per_chan;
	gr::thread::scoped_lock guard(d_mutex);



	// we use the open system call to get access to the O_LARGEFILE flag.  O_APPEND|
	int fd;
	if((fd = ::open(filename,
	                O_RDWR|O_CREAT|OUR_O_LARGEFILE|OUR_O_BINARY,
	                0664)) < 0) {
		perror(filename);
    std::cout << "wav error opening: " << filename << std::endl;
		return false;
	}

	if(d_new_fp) {    // if we've already got a new one open, close it
    std::cout << "d_new_fp alread open, closing "<< old_filename << " for " << filename << std::endl;
		fclose(d_new_fp);
		d_new_fp = 0;
	}
  strcpy(old_filename, filename);

	if((d_new_fp = fdopen (fd, "rb+")) == NULL) {
		perror(filename);
		::close(fd);  // don't leak file descriptor if fdopen fails.
    std::cout << "wav open failed" << std::endl;
		return false;
	}
	d_updated = true;


      // Scan headers, check file validity
      if(wavheader_parse(d_new_fp,
			  d_sample_rate,
			  d_nchans,
			  d_bytes_per_sample_new,
			  d_first_sample_pos,
			  d_samples_per_chan)) {


          d_sample_count = (unsigned) d_samples_per_chan * d_nchans;
          std::cout << "Wav: " << filename << " Existing Wav Sample Count: " << d_sample_count << " n_chans: " << d_nchans << " samples per_chan: " << d_samples_per_chan <<std::endl;
          //fprintf(stderr, "Existing Wav Sample Count: %d\n", d_sample_count);
          fseek(d_new_fp, 0, SEEK_END);

      } else {
          	d_sample_count = 0;
          // you have to rewind the d_new_fp because the read failed.

          if (fseek(d_new_fp, 0, SEEK_SET) != 0) {
            std::cout << "Wav parse header did not happen" << std::endl;
		      return false;
	       }
         std::cout << "Adding Wav header, bytes per sample: " << d_bytes_per_sample_new << std::endl;
        if(!wavheader_write(d_new_fp,
	                    d_sample_rate,
	                    d_nchans,
	                    d_bytes_per_sample_new)) {
		      fprintf(stderr, "[%s] could not write to WAV file\n", __FILE__);
		      exit(-1);
	       }
      }

      do_update();

	return true;
}

void
nonstop_wavfile_sink_impl::close()
{
	gr::thread::scoped_lock guard(d_mutex);

	if(!d_fp){
    std::cout << "wav error closing file" << std::endl;
    if (d_updated) {
      std::cout << "weird, update flagged" << std::endl;
    }
    if (d_new_fp) {
      std::cout << "weird, there is a d_new_fp" << std::endl;
    }
    d_new_fp = NULL;
		return;
  }

	close_wav();
}

void
nonstop_wavfile_sink_impl::close_wav()
{
	unsigned int byte_count = d_sample_count * d_bytes_per_sample;

  std::cout << "Closing wav byte count: " << byte_count << " samples: " << d_sample_count << " bytes per: " << d_bytes_per_sample << " new bytes per: " << d_bytes_per_sample_new << std::endl;
	wavheader_complete(d_fp, byte_count);

	fclose(d_fp);
	d_fp = NULL;
}

nonstop_wavfile_sink_impl::~nonstop_wavfile_sink_impl()
{
	if(d_new_fp) {
		fclose(d_new_fp);
		d_new_fp = NULL;
	}

	close();
}

bool nonstop_wavfile_sink_impl::stop()
{


	return true;
}

int
nonstop_wavfile_sink_impl::work(int noutput_items,
                                gr_vector_const_void_star &input_items,
                                gr_vector_void_star &output_items)
{
	float **in = (float**)&input_items[0];
	int n_in_chans = input_items.size();

	short int sample_buf_s;

	int nwritten;

	gr::thread::scoped_lock guard(d_mutex);    // hold mutex for duration of this block
	do_update();      // update: d_fp is reqd
	if(!d_fp)         // drop output on the floor
  {
    std::cout << "Wav - Dropping items, no fp: " << noutput_items << std::endl;
		return noutput_items;
  }
	for(nwritten = 0; nwritten < noutput_items; nwritten++) {
		for(int chan = 0; chan < d_nchans; chan++) {
			// Write zeros to channels which are in the WAV file
			// but don't have any inputs here
			if(chan < n_in_chans) {
				sample_buf_s =
				    convert_to_short(in[chan][nwritten]);
			}
			else {
				sample_buf_s = 0;
			}

			wav_write_sample(d_fp, sample_buf_s, d_bytes_per_sample);

			if(feof(d_fp) || ferror(d_fp)) {
				fprintf(stderr, "[%s] file i/o error\n", __FILE__);
				close();
				exit(-1);
			}
			d_sample_count++;
		}
	}

   // fflush (d_fp);  // this is added so unbuffered content is written.

	return nwritten;
}

short int
nonstop_wavfile_sink_impl::convert_to_short(float sample)
{
	sample += d_normalize_shift;
	sample *= d_normalize_fac;
	if(sample > d_max_sample_val) {
		sample = d_max_sample_val;
	}
	else if(sample < d_min_sample_val) {
		sample = d_min_sample_val;
	}

	return (short int)boost::math::iround(sample);
}

void
nonstop_wavfile_sink_impl::set_bits_per_sample(int bits_per_sample)
{
	gr::thread::scoped_lock guard(d_mutex);
	if(bits_per_sample == 8 || bits_per_sample == 16) {
		d_bytes_per_sample_new = bits_per_sample / 8;
	}
}

void
nonstop_wavfile_sink_impl::set_sample_rate(unsigned int sample_rate)
{
	gr::thread::scoped_lock guard(d_mutex);
	d_sample_rate = sample_rate;
}

int
nonstop_wavfile_sink_impl::bits_per_sample()
{
	return d_bytes_per_sample_new * 8;
}

unsigned int
nonstop_wavfile_sink_impl::sample_rate()
{
	return d_sample_rate;
}

double
nonstop_wavfile_sink_impl::length_in_seconds()
{
  std::cout << "Filename: "<< old_filename << "Sample #: " << d_sample_count << " rate: " << d_sample_rate << " bytes: " << d_bytes_per_sample_new << "\n";
  return (double) d_sample_count  / (double) d_sample_rate;
	//return (double) ( d_sample_count * d_bytes_per_sample_new * 8) / (double) d_sample_rate;
}

void
nonstop_wavfile_sink_impl::do_update()
{
  if ((!d_updated) && (!d_fp)) {
    std::cout << "Sample #: " << d_sample_count << " no update and no fp\n";

  }

	if(!d_updated) {
		return;
	}

	if(d_fp) {
		close_wav();
	}

	d_fp = d_new_fp;                    // install new file pointer
	d_new_fp  = 0;

	d_bytes_per_sample = d_bytes_per_sample_new;

	if(d_bytes_per_sample == 1) {
		d_max_sample_val = UCHAR_MAX;
		d_min_sample_val = 0;
		d_normalize_fac  = d_max_sample_val/2;
		d_normalize_shift = 1;
	}
	else if(d_bytes_per_sample == 2) {
		d_max_sample_val = SHRT_MAX;
		d_min_sample_val = SHRT_MIN;
		d_normalize_fac  = d_max_sample_val;
		d_normalize_shift = 0;
	}

	d_updated = false;
}

} /* namespace blocks */
} /* namespace gr */
