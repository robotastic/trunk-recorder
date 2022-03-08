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


#include "nonstop_wavfile_sink.h"
#include "nonstop_wavfile_sink_impl.h"
#include "../../trunk-recorder/call.h"
#include <boost/math/special_functions/round.hpp>
#include <climits>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/thread/thread.h>
#include <stdexcept>
#include <stdio.h>

// win32 (mingw/msvc) specific
#ifdef HAVE_IO_H
#include <io.h>
#endif // ifdef HAVE_IO_H
#ifdef O_BINARY
#define OUR_O_BINARY O_BINARY
#else // ifdef O_BINARY
#define OUR_O_BINARY 0
#endif // ifdef O_BINARY

// should be handled via configure
#ifdef O_LARGEFILE
#define OUR_O_LARGEFILE O_LARGEFILE
#else // ifdef O_LARGEFILE
#define OUR_O_LARGEFILE 0
#endif // ifdef O_LARGEFILE

namespace gr {
namespace blocks {
nonstop_wavfile_sink_impl::sptr
nonstop_wavfile_sink_impl::make(int n_channels, unsigned int sample_rate, int bits_per_sample) {
  return gnuradio::get_initial_sptr(new nonstop_wavfile_sink_impl(n_channels, sample_rate, bits_per_sample));
}

nonstop_wavfile_sink_impl::nonstop_wavfile_sink_impl(
    int n_channels,
    unsigned int sample_rate,
    int bits_per_sample)
    : sync_block("nonstop_wavfile_sink",
                 io_signature::make(1, n_channels, sizeof(int16_t)),
                 io_signature::make(0, 0, 0)),
      d_sample_rate(sample_rate), d_nchans(n_channels),
      d_fp(0), d_current_call(NULL) {
  if ((bits_per_sample != 8) && (bits_per_sample != 16)) {
    throw std::runtime_error("Invalid bits per sample (supports 8 and 16)");
  }
  d_bytes_per_sample = bits_per_sample / 8;
  d_sample_count = 0;
  d_slot = -1;
  d_first_work = true;
  d_termination_flag = false;
  state = AVAILABLE;
}

//static int rec_counter=0;
void nonstop_wavfile_sink_impl::create_base_filename() {
  time_t work_start_time = d_start_time;
  std::stringstream path_stream;
  tm *ltm = localtime(&work_start_time);
  // Found some good advice on Streams and Strings here: https://blog.sensecodons.com/2013/04/dont-let-stdstringstreamstrcstr-happen.html
  path_stream << d_current_call_capture_dir << "/" << d_current_call_short_name << "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;
  std::string path_string = path_stream.str();
  boost::filesystem::create_directories(path_string);

  int nchars;

  if (d_slot == -1) {
    nchars = snprintf(current_base_filename, 255, "%s/%ld-%ld_%.0f", path_string.c_str(), d_current_call_talkgroup, work_start_time, d_current_call_freq);
  } else {
    // this is for the case when it is a DMR recorder and 2 wav files are created, the slot is needed to keep them separate.
    nchars = snprintf(current_base_filename, 255, "%s/%ld-%ld_%.0f.%d", path_string.c_str(), d_current_call_talkgroup, work_start_time, d_current_call_freq,d_slot);
  }
  if (nchars >= 255) {
    BOOST_LOG_TRIVIAL(error) << "Call: Path longer than 255 charecters";
  }
}

char *nonstop_wavfile_sink_impl::get_filename() {
  return current_filename;
}

bool nonstop_wavfile_sink_impl::start_recording(Call *call, int slot) {
  this->d_slot = slot;
  this->start_recording(call);
  return true;
}

bool nonstop_wavfile_sink_impl::start_recording(Call *call) {
  gr::thread::scoped_lock guard(d_mutex);
  if (d_current_call && d_fp) {
    BOOST_LOG_TRIVIAL(trace) << "Start() - Current_Call & fp are not null! current_filename is: " << current_filename << " Length: " << d_sample_count << std::endl;
  } 
  d_current_call = call;
  d_current_call_num = call->get_call_num();
  d_current_call_recorder_num = 0; //call->get_recorder()->get_num();
  d_current_call_freq = call->get_freq();
  d_current_call_talkgroup = call->get_talkgroup();
  d_current_call_short_name = call->get_short_name();
  d_current_call_capture_dir = call->get_capture_dir();
  d_prior_transmission_length = 0;
  record_more_transmissions = true;

  this->clear_transmission_list();
  d_conventional = call->is_conventional();
  curr_src_id = d_current_call->get_current_source_id();
  d_sample_count = 0;
  d_first_work = true;

  // when a wav_sink first gets associated with a call, set its lifecycle to idle;
  state = IDLE;
  /* Should reset more variables here */

  char formattedTalkgroup[62];
  snprintf(formattedTalkgroup, 61, "%c[%dm%10ld%c[0m", 0x1B, 35, d_current_call_talkgroup, 0x1B);
  std::string talkgroup_display = boost::lexical_cast<std::string>(formattedTalkgroup);
  BOOST_LOG_TRIVIAL(trace) << "[" << d_current_call_short_name << "]\t\033[0;34m" << d_current_call_num << "C\033[0m\tTG: " << formattedTalkgroup << "\tFreq: " << format_freq(d_current_call_freq) << "\tStarting wavfile sink ";

  return true;
}

bool nonstop_wavfile_sink_impl::open_internal(const char *filename) {
  int d_first_sample_pos;
  unsigned d_samples_per_chan;

  // we use the open system call to get access to the O_LARGEFILE flag.
  //  O_APPEND|
  int fd;

  if ((fd = ::open(filename,
                   O_RDWR | O_CREAT | OUR_O_LARGEFILE | OUR_O_BINARY,
                   0664)) < 0) {
    perror(filename);
    BOOST_LOG_TRIVIAL(error) << "wav error opening: " << filename << std::endl;
    return false;
  }

  if (d_fp) { // if we've already got a new one open, close it
    BOOST_LOG_TRIVIAL(trace) << "File pointer already open, closing " << d_fp << " more" << current_filename << " for " << filename << std::endl;

    // fclose(d_fp);
    // d_fp = NULL;
  }

  if (strlen(filename) >= 255) {
    BOOST_LOG_TRIVIAL(error) << "nonstop_wavfile_sink: Error! filename longer than 255";
  }

  if ((d_fp = fdopen(fd, "rb+")) == NULL) {
    perror(filename);
    ::close(fd); // don't leak file descriptor if fdopen fails.
    BOOST_LOG_TRIVIAL(error) << "wav open failed" << std::endl;
    return false;
  }

  d_sample_count = 0;
  d_first_work = true;

  if (!wavheader_write(d_fp, d_sample_rate, d_nchans, d_bytes_per_sample)) {
    fprintf(stderr, "[%s] could not write to WAV file\n", __FILE__);
    return false;
  }

  if (d_bytes_per_sample == 1) {
    d_max_sample_val = UCHAR_MAX;
    d_min_sample_val = 0;
    d_normalize_fac = d_max_sample_val / 2;
    d_normalize_shift = 1;
  } else if (d_bytes_per_sample == 2) {
    d_max_sample_val = SHRT_MAX;
    d_min_sample_val = SHRT_MIN;
    d_normalize_fac = d_max_sample_val;
    d_normalize_shift = 0;
  }

  return true;
}

void nonstop_wavfile_sink_impl::set_source(long src) {
  gr::thread::scoped_lock guard(d_mutex);

  char formattedTalkgroup[62];
  snprintf(formattedTalkgroup, 61, "%c[%dm%10ld%c[0m", 0x1B, 35, d_current_call_talkgroup, 0x1B);
  std::string talkgroup_display = boost::lexical_cast<std::string>(formattedTalkgroup);

  if (curr_src_id == -1) {

    BOOST_LOG_TRIVIAL(error) << "[" << d_current_call_short_name << "]\t\033[0;34m" << d_current_call_num << "C\033[0m\tTG: " << formattedTalkgroup << "\tFreq: " << format_freq(d_current_call_freq) << "\tUnit ID externally set, ext: "<< src << "\tcurrent: " << curr_src_id << "\t samples: " << d_sample_count;

    curr_src_id = src;
  } else if (src != curr_src_id) {
    if (state == RECORDING) {

      BOOST_LOG_TRIVIAL(error) << "[" << d_current_call_short_name << "]\t\033[0;34m" << d_current_call_num << "C\033[0m\tTG: " << formattedTalkgroup << "\tFreq: " << format_freq(d_current_call_freq) << "\tUnit ID externally set, ext: "<< src << "\tcurrent: " << curr_src_id << "\t samples: " << d_sample_count;

      if (d_sample_count > 0) {
        end_transmission();
      }

      if (!record_more_transmissions) {
        state = STOPPED;
      } else {
        state = IDLE;
        d_first_work = true;
      }
    }
    curr_src_id = src;
  }
}

void nonstop_wavfile_sink_impl::end_transmission() {
  if (d_sample_count > 0) {
    if (d_fp) {
      close_wav(false);
    } else {
      BOOST_LOG_TRIVIAL(error) << "Ending transmission, sample_count is greater than 0 but d_fp is null" << std::endl;
    }
    // if an Transmission has ended, send it to Call.
    Transmission transmission;
    transmission.source = curr_src_id;      // Source ID for the Call
    transmission.start_time = d_start_time; // Start time of the Call
    transmission.stop_time = d_stop_time;   // when the Call eneded
    transmission.sample_count = d_sample_count;
    transmission.length = length_in_seconds();       // length in seconds
    d_prior_transmission_length = d_prior_transmission_length + transmission.length;
    strcpy(transmission.filename, current_filename); // Copy the filename
    strcpy(transmission.base_filename, current_base_filename);
    this->add_transmission(transmission);
    d_sample_count = 0;
    d_first_work = true;
  } else {
    BOOST_LOG_TRIVIAL(error) << "Trying to end a Transmission, but the sample_count is 0" << std::endl;
  }
}

void nonstop_wavfile_sink_impl::stop_recording() {
  gr::thread::scoped_lock guard(d_mutex);

  if (d_sample_count > 0) {
    end_transmission();
  }

  if (state == RECORDING) {
    BOOST_LOG_TRIVIAL(error) << "stop_recording() - stopping wavfile sink but recorder state is: " << state << std::endl;
  }
  d_current_call = NULL;
  d_first_work = true;
  d_termination_flag = false;
  state = AVAILABLE;
}

void nonstop_wavfile_sink_impl::close_wav(bool close_call) {
  unsigned int byte_count = d_sample_count * d_bytes_per_sample;
  wavheader_complete(d_fp, byte_count);
  fclose(d_fp);
  d_fp = NULL;
}

nonstop_wavfile_sink_impl::~nonstop_wavfile_sink_impl() {
  stop_recording();
}

bool nonstop_wavfile_sink_impl::stop() {
  return true;
}

State nonstop_wavfile_sink_impl::get_state() {
  return this->state;
}

int nonstop_wavfile_sink_impl::work(int noutput_items, gr_vector_const_void_star &input_items, gr_vector_void_star &output_items) {

  gr::thread::scoped_lock guard(d_mutex); // hold mutex for duration of this

  // it is possible that we could get part of a transmission after a call has stopped. We shouldn't do any recording if this happens.... this could mean that we miss part of the recording though
  if (!d_current_call) {
    time_t now = time(NULL);
    double its_been = difftime(now, d_stop_time);
    char formattedTalkgroup[62];
    snprintf(formattedTalkgroup, 61, "%c[%dm%10ld%c[0m", 0x1B, 35, d_current_call_talkgroup, 0x1B);
    std::string talkgroup_display = boost::lexical_cast<std::string>(formattedTalkgroup);
    BOOST_LOG_TRIVIAL(error) << "[" << d_current_call_short_name << "]\t\033[0;34m" << d_current_call_num << "C\033[0m\tTG: " << formattedTalkgroup << "\tFreq: " << format_freq(d_current_call_freq) << "\tDropping samples - current_call is null\t Rec State: " << format_state(this->state) << "\tSince close: " << its_been;

    return noutput_items;
  }

  // it is possible that we could get part of a transmission after a call has stopped. We shouldn't do any recording if this happens.... this could mean that we miss part of the recording though
  if ((state == STOPPED) || (state == AVAILABLE)) {
    if (noutput_items > 1) {
      time_t now = time(NULL);
      double its_been = difftime(now, d_stop_time);
      char formattedTalkgroup[62];
      snprintf(formattedTalkgroup, 61, "%c[%dm%10ld%c[0m", 0x1B, 35, d_current_call_talkgroup, 0x1B);
      std::string talkgroup_display = boost::lexical_cast<std::string>(formattedTalkgroup);
      BOOST_LOG_TRIVIAL(error) << "[" << d_current_call_short_name << "]\t\033[0;34m" << d_current_call_num << "C\033[0m\tTG: " << formattedTalkgroup << "\tFreq: " << format_freq(d_current_call_freq) << "\tDropping samples - Recorder state is: " << format_state(this->state);

      //BOOST_LOG_TRIVIAL(info) << "WAV - state is: " << format_state(this->state) << "\t Dropping samples: " << noutput_items << " Since close: " << its_been << std::endl;
    }
    return noutput_items;
  }

  std::vector<gr::tag_t> tags;
  pmt::pmt_t this_key(pmt::intern("src_id"));
  pmt::pmt_t that_key(pmt::intern("terminate"));
  //pmt::pmt_t squelch_key(pmt::intern("squelch_eob"));
  get_tags_in_range(tags, 0, nitems_read(0), nitems_read(0) + noutput_items);

  unsigned pos = 0;
  //long curr_src_id = 0;

  for (unsigned int i = 0; i < tags.size(); i++) {
    //BOOST_LOG_TRIVIAL(info) << "TAG! " << tags[i].key;
    if (pmt::eq(this_key, tags[i].key)) {
      long src_id = pmt::to_long(tags[i].value);
      pos = d_sample_count + (tags[i].offset - nitems_read(0));

      if (curr_src_id == -1) {
        BOOST_LOG_TRIVIAL(info) << "Updated Voice Channel source id: " << src_id << " pos: " << pos << " offset: " << tags[i].offset - nitems_read(0);
        
        curr_src_id = src_id;
      } else if (src_id != curr_src_id) {
        if (state == RECORDING) {

          if (d_sample_count > 0) {
            end_transmission();
          }

          if (!record_more_transmissions) {
            state = STOPPED;
          } else {
            state = IDLE;
            d_first_work = true;
          }
        }
        BOOST_LOG_TRIVIAL(info) << "Updated Voice Channel source id: " << src_id << " pos: " << pos << " offset: " << tags[i].offset - nitems_read(0);
        
        curr_src_id = src_id;
      }

    }
    if (pmt::eq(that_key, tags[i].key)) {
      d_termination_flag = true;
    }
  }
  tags.clear();

  // if the System for this call is in Transmission Mode, and we have a recording and we got a flag that a Transmission ended...
  int nwritten = dowork(noutput_items, input_items, output_items);
  d_stop_time = time(NULL);

  return nwritten;
}

time_t nonstop_wavfile_sink_impl::get_start_time() {
  return d_start_time;
}

time_t nonstop_wavfile_sink_impl::get_stop_time() {
  return d_stop_time;
}

void nonstop_wavfile_sink_impl::add_transmission(Transmission t) {
  transmission_list.push_back(t);
}

void nonstop_wavfile_sink_impl::set_record_more_transmissions(bool more) {
  record_more_transmissions = more;
}

void nonstop_wavfile_sink_impl::clear_transmission_list() {
  transmission_list.clear();
  transmission_list.shrink_to_fit();
}

std::vector<Transmission> nonstop_wavfile_sink_impl::get_transmission_list() {
  return transmission_list;
}

int nonstop_wavfile_sink_impl::dowork(int noutput_items, gr_vector_const_void_star &input_items, gr_vector_void_star &output_items) {
  // block
  int n_in_chans = input_items.size();
  int16_t sample_buf_s;
  int nwritten;

  
  if (d_termination_flag) {

    if (d_current_call == NULL) {
      BOOST_LOG_TRIVIAL(error) << "wav - no current call in temination loop";
      state = STOPPED;
      return noutput_items;
    }

    if (d_sample_count > 0) {
      end_transmission();
    }
    if (!record_more_transmissions) {
      char formattedTalkgroup[62];
      snprintf(formattedTalkgroup, 61, "%c[%dm%10ld%c[0m", 0x1B, 35, d_current_call_talkgroup, 0x1B);
      std::string talkgroup_display = boost::lexical_cast<std::string>(formattedTalkgroup);
      
      BOOST_LOG_TRIVIAL(trace) << "[" << d_current_call_short_name << "]\t\033[0;34m" << d_current_call_num << "C\033[0m\tTG: " << formattedTalkgroup << "\tFreq: " << format_freq(d_current_call_freq) << "\trecord_more_transmissions is false, setting recorder state to STOPPED";
      BOOST_LOG_TRIVIAL(trace) << "Call completed - putting recorder into state Completed - we had samples";
      
      state = STOPPED;
    } else {
      state = IDLE;
      d_first_work = true;
    }
    d_termination_flag = false;

    return noutput_items;
  }

  if (d_first_work) {
    if (d_fp) {
      // if we are already recording a file for this call, close it before starting a new one.
      BOOST_LOG_TRIVIAL(info) << "WAV - Weird! we have an existing FP, but d_first_work was true:  " << current_filename << std::endl;

      close_wav(false);
    }

    time_t current_time = time(NULL);
    if (current_time == d_start_time) {
      d_start_time = current_time + 1;
    } else {
      d_start_time = current_time;
    }

    // create a new filename, based on the current time and source.
    create_base_filename();
    strcpy(current_filename, current_base_filename);
    strcat(current_filename, ".wav");
    if (!open_internal(current_filename)) {
      BOOST_LOG_TRIVIAL(error) << "can't open file";
      return noutput_items;
    }
    char formattedTalkgroup[62];
    snprintf(formattedTalkgroup, 61, "%c[%dm%10ld%c[0m", 0x1B, 35, d_current_call_talkgroup, 0x1B);
    std::string talkgroup_display = boost::lexical_cast<std::string>(formattedTalkgroup);
    BOOST_LOG_TRIVIAL(info) << "[" << d_current_call_short_name << "]\t\033[0;34m" << d_current_call_num << "C\033[0m\tTG: " << formattedTalkgroup << "\tFreq: " << format_freq(d_current_call_freq) << "\tStarting new Transmission \tSrc ID:  " << curr_src_id;

    //curr_src_id = d_current_call->get_current_source_id();

    d_first_work = false;
  }

  if (!d_fp) // drop output on the floor
  {
    BOOST_LOG_TRIVIAL(error) << "Wav - Dropping items, no fp or Current Call: " << noutput_items << " Filename: " << current_filename << " Current sample count: " << d_sample_count << std::endl;
    return noutput_items;
  }

  for (nwritten = 0; nwritten < noutput_items; nwritten++) {
    for (int chan = 0; chan < d_nchans; chan++) {
      // Write zeros to channels which are in the WAV file
      // but don't have any inputs here
      if (chan < n_in_chans) {
          int16_t **in = (int16_t **)&input_items[0];
          sample_buf_s = in[chan][nwritten];
      } else {
        sample_buf_s = 0;
      }

      wav_write_sample(d_fp, sample_buf_s, d_bytes_per_sample);

      d_sample_count++;
    }
  }

  if (nwritten > 0) {
    state = RECORDING;
  }
  // fflush (d_fp);  // this is added so unbuffered content is written.
  return nwritten;
}

void nonstop_wavfile_sink_impl::set_bits_per_sample(int bits_per_sample) {
  gr::thread::scoped_lock guard(d_mutex);

  if ((bits_per_sample == 8) || (bits_per_sample == 16)) {
    d_bytes_per_sample = bits_per_sample / 8;
  }
}

void nonstop_wavfile_sink_impl::set_sample_rate(unsigned int sample_rate) {
  gr::thread::scoped_lock guard(d_mutex);

  d_sample_rate = sample_rate;
}

int nonstop_wavfile_sink_impl::bits_per_sample() {
  return d_bytes_per_sample * 8;
}

unsigned int
nonstop_wavfile_sink_impl::sample_rate() {
  return d_sample_rate;
}

double nonstop_wavfile_sink_impl::total_length_in_seconds() {
  return this->length_in_seconds() + d_prior_transmission_length;
}

double nonstop_wavfile_sink_impl::length_in_seconds() {
  // std::cout << "Filename: "<< current_filename << "Sample #: " <<
  // d_sample_count << " rate: " << d_sample_rate << " bytes: " <<
  // d_bytes_per_sample << "\n";
  return (double)d_sample_count / (double)d_sample_rate;

  // return (double) ( d_sample_count * d_bytes_per_sample_new * 8) / (double)
  // d_sample_rate;
}

void nonstop_wavfile_sink_impl::do_update() {}
} /* namespace blocks */
} /* namespace gr */
