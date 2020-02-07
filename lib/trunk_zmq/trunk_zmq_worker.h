#ifndef _TRUNK_ZMQ_WORKER_H_
#define _TRUNK_ZMQ_WORKER_H_

#include <zmq.hpp>
#include <string>
#include <boost/log/trivial.hpp>

#define INPROC_ADDR "inproc://workers"

namespace trunk_zmq {
	class trunk_zmq_worker
	{
	public:
		trunk_zmq_worker(std::string worker_type);
		~trunk_zmq_worker();

		bool connect_worker(zmq::context_t& context);
		std::string get_worker_type();
	private:
		zmq::socket_t* d_worker;
		bool d_zmq_running;
		std::string d_worker_type;

	protected:
		int broadcast_message(const char* msg);

		virtual void connect_child_workers(zmq::context_t& context) {};
	};
}
#endif // _TRUNK_ZMQ_WORKER_H_