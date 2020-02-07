#ifndef _TRUNK_ZMQ_WORKER_H_
#define _TRUNK_ZMQ_WORKER_H_

#include <zmq.hpp>
#include <string>
#include <boost/log/trivial.hpp>
#include <gnuradio/blocks/api.h>
#include "trunk_zmq_common.h"

namespace gr {
	namespace blocks {
		namespace trunk_zmq {
			class BLOCKS_API trunk_zmq_worker
			{
			public:
				trunk_zmq_worker();
				~trunk_zmq_worker();

				bool connect_worker(zmq::context_t& context);
				std::string get_worker_type();
				void set_worker_type(std::string worker_type);
			private:
				zmq::socket_t* d_worker;
				bool d_zmq_running;
				std::string d_worker_type;

			protected:
				int broadcast_message(const char* msg);
				int broadcast_decoder_msg(std::string system_name, long unitId, const char* system_type, bool emergency, int unit_id_hex_digits);

				virtual void connect_child_workers(zmq::context_t& context);
			};
		}
	}
}
#endif // _TRUNK_ZMQ_WORKER_H_