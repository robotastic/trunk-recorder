#ifndef INCLUDED_MP3_FILE_DELAYOPEN_SINK_IMPL_H
#define INCLUDED_MP3_FILE_DELAYOPEN_SINK_IMPL_H

#include "./mp3_file_sink_impl.h"
#include "./mp3_file_sink.h"

class p25conventional_recorder;

namespace gr {
	namespace blocks {

		class mp3_file_delayopen_sink_impl : public mp3_file_sink_impl
		{
		public:

			typedef boost::shared_ptr<mp3_file_delayopen_sink_impl> sptr;

			/*
			 * \param filename The .wav file to be opened
			 * \param n_channels Number of channels (2 = stereo or I/Q output)
			 * \param sample_rate Sample rate [S/s]
			 * \param bits_per_sample 16 or 8 bit, default is 16
			 */
			static sptr make(int n_channels,
				unsigned int sample_rate,
				int bits_per_sample = 16);

			mp3_file_delayopen_sink_impl(int n_channels,
				unsigned int sample_rate,
				int bits_per_sample);

			virtual ~mp3_file_delayopen_sink_impl();

			virtual void close();

			int work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items);

			time_t get_start_time();
			time_t get_stop_time();
			void set_recorder(Recorder* recorder);

			void reset();

		private:
			bool d_delay_file_creation;
			bool d_first_work;
			time_t d_start_time;
			clock_t d_last_packet_clock;
			time_t d_stop_time;
			Recorder* recorder;

		};

	} /* namespace blocks */
} /* namespace gr */

#endif /* INCLUDED_MP3_FILE_DELAYOPEN_SINK_IMPL_H */
