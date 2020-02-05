#ifndef _TRUNK_ZMQ_CORE_H_
#define _TRUNK_ZMQ_CORE_H_

#include <zmq.hpp>

class trunk_zmq_core
{
public:
	trunk_zmq_core(const char* bind_addr);
	~trunk_zmq_core();

	static const char* INPROC_ADDR;

	zmq::context_t& get_context();

	void start();
private:
	zmq::context_t d_context;
	zmq::socket_t* d_clients;
	zmq::socket_t* d_workers;
};

#endif // _TRUNK_ZMQ_CORE_H_