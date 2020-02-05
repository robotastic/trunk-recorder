#include "trunk_zmq_core.h"

const char* trunk_zmq_core::INPROC_ADDR = "inproc://workers";

trunk_zmq_core::trunk_zmq_core(const char* bind_addr)
{
	d_context = zmq::context_t();

	// Public connections
	d_clients = new zmq::socket_t(d_context, ZMQ_XPUB);
	d_clients->bind(bind_addr);

	// Internal connections
	d_workers = new zmq::socket_t(d_context, ZMQ_XSUB);
	d_workers->bind(INPROC_ADDR);
}

trunk_zmq_core::~trunk_zmq_core()
{
	zmq_close(d_clients);
	
	zmq_ctx_destroy(&d_context);
}

zmq::context_t& trunk_zmq_core::get_context()
{
	return d_context;
}

void trunk_zmq_core::start()
{
	zmq::proxy(static_cast<void*>(d_clients), static_cast<void*>(d_workers), nullptr);
}