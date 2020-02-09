#ifndef _trunk_core_H_
#define _trunk_core_H_

#include <string>
#include <queue>
#include <boost/log/trivial.hpp>
#include <boost/thread.hpp>
#include <gnuradio/blocks/api.h>

namespace gr {
	namespace blocks {
		namespace trunk_ctrl {

			class BLOCKS_API trunk_core
			{
			public:
				trunk_core(const char* bind_addr);
				~trunk_core();

				bool start();
				void stop();
				bool is_running();

				int broadcast_message(const char* msg);
			private:
				pthread_t d_background_thread;
				bool d_should_run;
				boost::mutex d_mutex;
				std::queue<std::string> d_queue;

				static void* _do_background_thread(void* ctx);
				void do_background_thread();

				bool pop_msg(std::string& msg);
			};
		}
	}
}
#endif // _trunk_core_H_
