#include <string>
#include <sstream>

#include "trunk_zmq_worker.h"
#include "zhelpers.h"

namespace gr {
	namespace blocks {
		namespace trunk_zmq {

			trunk_zmq_worker::trunk_zmq_worker()
				: d_zmq_running(false)
			{
				//
			}

			trunk_zmq_worker::~trunk_zmq_worker()
			{
				zmq_close(d_worker);
			}

			std::string trunk_zmq_worker::get_worker_type()
			{
				return d_worker_type;
			}
			void trunk_zmq_worker::set_worker_type(std::string worker_type)
			{
				d_worker_type = worker_type;
			}

			bool trunk_zmq_worker::connect_worker(zmq::context_t& context)
			{
				if (d_zmq_running) return true;

				try {
					BOOST_LOG_TRIVIAL(trace) << "ZMQ_WORKER: Connecting " << get_worker_type() << " to ZMQ Core";

					// Worker socket, publishes to the trunk_zmq_core::d_workers
					d_worker = new zmq::socket_t(context, ZMQ_PUSH);
					d_worker->bind(INPROC_WORKER_ADDR);

					connect_child_workers(context);

					d_zmq_running = true;
				}
				catch (std::exception const& e)
				{
					BOOST_LOG_TRIVIAL(error) << "ZMQ_WORKER: ERROR CONNECTING: " << e.what();
					d_zmq_running = false;
				}
				return d_zmq_running;
			}

			void trunk_zmq_worker::connect_child_workers(zmq::context_t& context) {}

			int trunk_zmq_worker::broadcast_message(const char* msg)
			{
				if (d_zmq_running)
				{
					BOOST_LOG_TRIVIAL(info) << "Broadcast: " << msg;
					return s_send(d_worker, msg);
				}
				else
				{
					BOOST_LOG_TRIVIAL(warning) << "Unable to Broadcast: " << msg;
					return 0;
				}
			}

			int trunk_zmq_worker::broadcast_decoder_msg(std::string system_name, long unitId, const char* system_type, bool emergency, int unit_id_hex_digits)
			{
				std::stringstream ss;

				ss << "SIGNAL|" << system_name << "|" << system_type << "|";

				if (unit_id_hex_digits > 0)
				{
					ss << std::setfill('0') << std::setw(unit_id_hex_digits) << std::right << std::hex << unitId;
				}
				else
				{
					ss << unitId;
				}
				ss << "|EMER=" << (emergency ? "Y" : "N");

				return broadcast_message(ss.str().c_str());
			}
		}
	}
}