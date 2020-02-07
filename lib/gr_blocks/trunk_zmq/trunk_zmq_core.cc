#include "trunk_zmq_core.h"
#include "trunk_zmq_worker.h"
#include "trunk_zmq_common.h"

namespace gr {
	namespace blocks {
		namespace trunk_zmq {

			trunk_zmq_core::trunk_zmq_core(const char* bind_addr)
				: d_background_thread(0)
			{
				BOOST_LOG_TRIVIAL(info) << "Creating trunk_zmq_core: " << bind_addr;

				d_context = zmq::context_t();

				BOOST_LOG_TRIVIAL(info) << "ZMQ Context created, creating XPUB";

				// Public connections
				d_clients = new zmq::socket_t(d_context, ZMQ_XPUB);
				d_clients->bind(bind_addr);

				BOOST_LOG_TRIVIAL(info) << "ZMQ XPUB created, creating XSUB";

				// Internal worker connections
				d_workers = new zmq::socket_t(d_context, ZMQ_XSUB);
				d_workers->bind(INPROC_WORKER_ADDR);

				BOOST_LOG_TRIVIAL(info) << "ZMQ XSUB created, creating PUB";

				// Internal worker capture connections
				d_worker_capture = new zmq::socket_t(d_context, ZMQ_PUB);
				d_worker_capture->bind(INPROC_WORKER_CAPTURE_ADDR);

				BOOST_LOG_TRIVIAL(info) << "ZMQ Core Setup!";
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
				zmq::proxy(static_cast<void*>(d_clients), static_cast<void*>(d_workers), static_cast<void*>(d_worker_capture));
			}

			bool trunk_zmq_core::start()
			{
				if (is_running()) return false;

				BOOST_LOG_TRIVIAL(info) << "Starting ZMQ Core";
				pthread_create(&d_background_thread, NULL, _do_background_thread, (void*)this);

				return true;
			}

			void trunk_zmq_core::stop()
			{
				if (!is_running()) return;

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