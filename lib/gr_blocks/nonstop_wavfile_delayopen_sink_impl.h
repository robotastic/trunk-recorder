#ifndef INCLUDED_NONSTOP_WAVFILE_DELAYOPEN_SINK_IMPL_H
#define INCLUDED_NONSTOP_WAVFILE_DELAYOPEN_SINK_IMPL_H

#include "./nonstop_wavfile_sink_impl.h"
#include "./nonstop_wavfile_sink.h"

class p25conventional_recorder;

namespace gr {
namespace blocks {

class nonstop_wavfile_delayopen_sink_impl : public nonstop_wavfile_sink_impl
{
public:

	typedef boost::shared_ptr<nonstop_wavfile_delayopen_sink_impl> sptr;

	/*
	 * \param filename The .wav file to be opened
	 * \param n_channels Number of channels (2 = stereo or I/Q output)
	 * \param sample_rate Sample rate [S/s]
	 * \param bits_per_sample 16 or 8 bit, default is 16
	 */
	static sptr make(int n_channels,
	                 unsigned int sample_rate,
	                 int bits_per_sample = 16,
								 		bool use_float=true);

	nonstop_wavfile_delayopen_sink_impl(int n_channels,
								unsigned int sample_rate,
	                          	int bits_per_sample,
								bool use_float);

	virtual ~nonstop_wavfile_delayopen_sink_impl();

	virtual void close();

	int work(int noutput_items,  gr_vector_const_void_star& input_items,  gr_vector_void_star& output_items);

	time_t get_start_time();
	time_t get_stop_time();
	void set_recorder(p25conventional_recorder * recorder);

	void reset();

private:
	bool d_delay_file_creation;
	bool d_first_work;
	time_t d_start_time;
	clock_t d_last_packet_clock;
	time_t d_stop_time;
	p25conventional_recorder * recorder;

};

} /* namespace blocks */
} /* namespace gr */

#endif /* INCLUDED_NONSTOP_WAVFILE_DELAYOPEN_SINK_IMPL_H */
