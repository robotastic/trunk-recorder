#include <string>
#include <sstream>

#include "trunk_worker.h"
#include "trunk_core.h"
#include "zhelpers.h"

namespace gr {
	namespace blocks {
		namespace trunk_ctrl {

			trunk_worker::trunk_worker()
				: d_zmq_running(false)
			{
				d_core = NULL;
			}

			trunk_worker::~trunk_worker()
			{
				//
			}

			std::string trunk_worker::get_worker_type()
			{
				return d_worker_type;
			}
			void trunk_worker::set_worker_type(std::string worker_type)
			{
				d_worker_type = worker_type;
			}

			bool trunk_worker::connect_worker(trunk_core* context)
			{
				if (d_zmq_running) return true;
				if (context == NULL) return false;

				try {
					BOOST_LOG_TRIVIAL(info) << "ZMQ_WORKER: Connecting " << get_worker_type() << " to ZMQ Core";

					d_core = context;

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

			int trunk_worker::broadcast_message(const char* msg)
			{
				if (d_zmq_running)
				{
					BOOST_LOG_TRIVIAL(info) << "Broadcast: " << msg;
					return d_core->broadcast_message(msg);
				}
				else
				{
					BOOST_LOG_TRIVIAL(warning) << "Unable to Broadcast: " << msg;
					return 0;
				}
			}

			int trunk_worker::broadcast_decoder_msg(std::string system_name, long unitId, const char* system_type, bool emergency, int unit_id_hex_digits)
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