#ifndef _trunk_worker_H_
#define _trunk_worker_H_

#include <zmq.hpp>
#include <string>
#include <boost/log/trivial.hpp>
#include <gnuradio/blocks/api.h>
#include "trunk_common.h"
#include "trunk_core.h"

namespace gr {
	namespace blocks {
		namespace trunk_ctrl {
			class BLOCKS_API trunk_worker
			{
			public:
				trunk_worker();
				~trunk_worker();

				bool connect_worker(trunk_core* context);
				std::string get_worker_type();
				void set_worker_type(std::string worker_type);
			private:
				bool d_zmq_running;
				std::string d_worker_type;
				trunk_core* d_core;

			protected:
				int broadcast_message(const char* msg);
				int broadcast_decoder_msg(std::string system_name, long unitId, const char* system_type, bool emergency, int unit_id_hex_digits);

				virtual void connect_child_workers(trunk_core* context) {};
			};
		}
	}
}
#endif // _trunk_worker_H_