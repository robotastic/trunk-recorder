#include <boost/thread.hpp>

#include "trunk_core.h"
#include "trunk_worker.h"
#include "trunk_common.h"
#include "zhelpers.h"

namespace gr {
	namespace blocks {
		namespace trunk_ctrl {

			trunk_core::trunk_core(const char* bind_addr)
				: d_background_thread(0),
				d_should_run(false)
			{
				try
				{
					BOOST_LOG_TRIVIAL(info) << "Creating trunk_core: " << bind_addr;

					d_queue.push("QUEUE CREATED");

					BOOST_LOG_TRIVIAL(info) << "ZMQ Core Setup!";
				}
				catch (std::exception const& e)
				{
					BOOST_LOG_TRIVIAL(error) << "ZMQ Core Error: " << e.what();
				}
			}

			trunk_core::~trunk_core()
			{
				//
			}

			void* trunk_core::_do_background_thread(void* ctx)
			{
				trunk_core* obj = (trunk_core*)ctx;
				obj->do_background_thread();
				return (NULL);
			}

			bool trunk_core::pop_msg(std::string& msg)
			{
				boost::lock_guard<boost::mutex> guard(d_mutex);

				if (d_queue.empty())
				{
					return false;
				}
				else {
					msg = d_queue.front();
					d_queue.pop();

					return true;
				}
			}
			void trunk_core::do_background_thread()
			{
				BOOST_LOG_TRIVIAL(info) << "ZMQ_CORE Background thread started.";
				while (d_should_run) {

					std::string msg;
					if (pop_msg(msg)) {
						BOOST_LOG_TRIVIAL(info) << "CORE MSG: " << msg;
					}
					else {
						boost::this_thread::sleep(boost::posix_time::milliseconds(100));
					}
				}
				BOOST_LOG_TRIVIAL(info) << "ZMQ_CORE Background thread stopped.";
			}

			bool trunk_core::start()
			{
				if (is_running()) return false;
				d_should_run = true;

				BOOST_LOG_TRIVIAL(info) << "Starting ZMQ Core";
				pthread_create(&d_background_thread, NULL, _do_background_thread, (void*)this);

				return true;
			}

			void trunk_core::stop()
			{
				if (!is_running()) return;

				BOOST_LOG_TRIVIAL(info) << "Stopping ZMQ Core";

				d_should_run = false;

				boost::this_thread::sleep(boost::posix_time::milliseconds(250));

				//need to stop d_background_thread
				d_background_thread = 0;
			}

			bool trunk_core::is_running()
			{
				return d_background_thread != 0;
			}

			int trunk_core::broadcast_message(const char* msg)
			{
				if (msg == NULL) return 0;

				boost::lock_guard<boost::mutex> guard(d_mutex);
				d_queue.push(std::string(msg));

				return strlen(msg);
			}
		}
	}
}