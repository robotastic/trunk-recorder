#ifndef _TRUNK_ZMQ_CORE_H_
#define _TRUNK_ZMQ_CORE_H_

#include <zmq.hpp>
#include <boost/log/trivial.hpp>
#include "trunk_zmq_worker.h"

class trunk_zmq_core
{
public:
	trunk_zmq_core(const char* bind_addr);
	~trunk_zmq_core();

	static const char* INPROC_ADDR;

	zmq::context_t& get_context();

	bool start();
	void stop();
	bool is_running();

	bool register_worker(trunk_zmq_worker* worker);
private:
	zmq::context_t d_context;
	zmq::socket_t* d_clients;
	zmq::socket_t* d_workers;
	pthread_t d_background_thread;

	static void* _do_background_thread(void* ctx);
	void do_background_thread();
};

#endif // _TRUNK_ZMQ_CORE_H_