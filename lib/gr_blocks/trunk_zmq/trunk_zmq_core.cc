#include <boost/thread.hpp>

#include "trunk_zmq_core.h"
#include "trunk_zmq_worker.h"
#include "trunk_zmq_common.h"
#include "zhelpers.h"

namespace gr {
	namespace blocks {
		namespace trunk_zmq {

			trunk_zmq_core::trunk_zmq_core(const char* bind_addr)
				: d_background_thread(0),
				d_should_run(false)
			{
				try
				{
					BOOST_LOG_TRIVIAL(info) << "Creating trunk_zmq_core: " << bind_addr;

					d_context = zmq::context_t();

					BOOST_LOG_TRIVIAL(info) << "ZMQ Context created, creating ZMQ_PULL";

					// Public connections
					d_clients = new zmq::socket_t(d_context, ZMQ_PULL);
					d_clients->bind(INPROC_WORKER_ADDR);

					BOOST_LOG_TRIVIAL(info) << "ZMQ_PULL created, creating ZMQ_PUB";

					d_pub_server = new zmq::socket_t(d_context, ZMQ_PUB);
					d_pub_server->bind(PUBLISH_ADDR);

					BOOST_LOG_TRIVIAL(info) << "ZMQ Core Setup!";
				}
				catch (std::exception const& e)
				{
					BOOST_LOG_TRIVIAL(error) << "ZMQ Core Error: " << e.what();
				}
			}

			trunk_zmq_core::~trunk_zmq_core()
			{
				zmq_close(d_clients);

				zmq_ctx_destroy(&d_context);
			}

			zmq::context_t& trunk_zmq_core::get_context()
			{
				return d_context;
			}

			void* trunk_zmq_core::_do_background_thread(void* ctx)
			{
				trunk_zmq_core* obj = (trunk_zmq_core*)ctx;
				obj->do_background_thread();
				return (NULL);
			}

			void trunk_zmq_core::do_background_thread()
			{
				BOOST_LOG_TRIVIAL(info) << "ZMQ_CORE Background thread started.";
				while (d_should_run) {
					char* msg = s_recv(d_clients, ZMQ_NOBLOCK);
					if (msg != nullptr) {
						BOOST_LOG_TRIVIAL(info) << "CORE MSG: " << msg;
						s_send(d_pub_server, msg, ZMQ_NOBLOCK);
						free(msg);
					} else {
						boost::this_thread::sleep(boost::posix_time::milliseconds(100));
					}
				}
				BOOST_LOG_TRIVIAL(info) << "ZMQ_CORE Background thread stopped.";
			}

			bool trunk_zmq_core::start()
			{
				if (is_running()) return false;
				d_should_run = true;

				BOOST_LOG_TRIVIAL(info) << "Starting ZMQ Core";
				pthread_create(&d_background_thread, NULL, _do_background_thread, (void*)this);

				return true;
			}

			void trunk_zmq_core::stop()
			{
				if (!is_running()) return;

				d_should_run = false;

				BOOST_LOG_TRIVIAL(info) << "Stopping ZMQ Core";

				//need to stop d_background_thread
				d_background_thread = 0;
			}

			bool trunk_zmq_core::is_running()
			{
				return d_background_thread != 0;
			}

			bool trunk_zmq_core::register_worker(trunk_zmq_worker* worker)
			{
				if (worker == NULL) return false;

				return worker->connect_worker(get_context());
			}
		}
	}
}