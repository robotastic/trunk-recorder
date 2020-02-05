#include "trunk_zmq_worker.h"
#include "trunk_zmq_core.h"
#include "zhelpers.h"

trunk_zmq_worker::trunk_zmq_worker(zmq::context_t& context)
{
	// Worker socket, publishes to the trunk_zmq_core::d_workers
	d_worker = new zmq::socket_t(context, ZMQ_PUB);
	d_worker->bind(trunk_zmq_core::INPROC_ADDR);
}

trunk_zmq_worker::~trunk_zmq_worker()
{
	zmq_close(d_worker);
}

int trunk_zmq_worker::broadcast_message(const char* msg)
{
	return s_send(d_worker, msg);
}