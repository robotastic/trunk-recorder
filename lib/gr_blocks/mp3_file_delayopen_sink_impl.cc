
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif // ifdef HAVE_CONFIG_H

#include <gnuradio/io_signature.h>

#include "mp3_file_delayopen_sink_impl.h"
#include "../trunk-recorder/recorders/p25conventional_recorder.h"

namespace gr
{
    namespace blocks
    {
        mp3_file_delayopen_sink_impl::sptr
            mp3_file_delayopen_sink_impl::make(int          n_channels,
                unsigned int sample_rate,
                int          bits_per_sample)
        {
            return gnuradio::get_initial_sptr
            (new mp3_file_delayopen_sink_impl(n_channels,
                sample_rate, bits_per_sample));
        }

        mp3_file_delayopen_sink_impl::mp3_file_delayopen_sink_impl(
            int          n_channels,
            unsigned int sample_rate,
            int          bits_per_sample)
            :sync_block("mp3_file_delayopen_sink",
                io_signature::make(1, n_channels, sizeof(float)),
                io_signature::make(0, 0, 0)),
            mp3_file_sink_impl(n_channels, sample_rate, bits_per_sample)
        {
            d_first_work = true;
            d_sample_count = 0;
        }

        mp3_file_delayopen_sink_impl::~mp3_file_delayopen_sink_impl()
        {
        }

        void mp3_file_delayopen_sink_impl::reset()
        {
            d_first_work = true;
            d_sample_count = 0;
        }

        int mp3_file_delayopen_sink_impl::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items) {

            gr::thread::scoped_lock guard(d_mutex); // hold mutex for duration of this

            bool was_first_work = d_first_work;
            d_first_work = false;

            if (!d_fp) // drop output on the floor
            {
                if (was_first_work) {
                    char* filename = recorder->get_filename();
                    if (!open_internal(filename)) {
                        throw std::runtime_error("can't open file");
                    }
                }
            }

            if (was_first_work) {
                d_start_time = time(NULL);
                recorder->recording_started();
            }

            // insert delay
            /*
            if (was_first_work == false) {
                if ((clock()- d_last_packet_clock) > 400)
                {
                    int pad_ms = clock()- d_last_packet_clock;

                }
            }*/

            int nwritten = dowork(noutput_items, input_items, output_items);

            d_stop_time = time(NULL);
            d_last_packet_clock = clock();

            return nwritten;
            //return 0;
        }

        time_t mp3_file_delayopen_sink_impl::get_start_time() {
            return d_start_time;
        }

        time_t mp3_file_delayopen_sink_impl::get_stop_time() {
            return d_stop_time;
        }

        void mp3_file_delayopen_sink_impl::set_recorder(p25conventional_recorder* recorder)
        {
            this->recorder = recorder;
        }

        void
            mp3_file_delayopen_sink_impl::close()
        {
            gr::thread::scoped_lock guard(d_mutex);

            if (!d_fp) {
                if (d_first_work == false) {
                    BOOST_LOG_TRIVIAL(error) << "mp3 error closing file" << std::endl;
                }
                return;
            }
            close_mp3();
        }

    } /* namespace blocks */
} /* namespace gr */
