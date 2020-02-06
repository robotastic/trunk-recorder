#ifndef _TRUNK_ZMQ_WORKER_H_
#define _TRUNK_ZMQ_WORKER_H_

#include <zmq.hpp>
#include <boost/log/trivial.hpp>

class trunk_zmq_worker
{
public:
	trunk_zmq_worker();
	~trunk_zmq_worker();

	bool connect_worker(zmq::context_t& context);
private:
	zmq::socket_t* d_worker;
	bool d_zmq_running;

protected:
	int broadcast_message(const char* msg);
};

#endif // _TRUNK_ZMQ_WORKER_H_