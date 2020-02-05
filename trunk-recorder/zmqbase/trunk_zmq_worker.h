#ifndef _TRUNK_ZMQ_WORKER_H_
#define _TRUNK_ZMQ_WORKER_H_

#include <zmq.hpp>

class trunk_zmq_worker
{
public:
	trunk_zmq_worker(zmq::context_t& context);
	~trunk_zmq_worker();

protected:
	zmq::socket_t* d_worker;

	int broadcast_message(const char* msg);
};

#endif // _TRUNK_ZMQ_WORKER_H_