#include "trunk_zmq_core.h"

const char* trunk_zmq_core::INPROC_ADDR = "inproc://workers";

trunk_zmq_core::trunk_zmq_core(const char* bind_addr)
	: d_background_thread(0)
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

void* trunk_zmq_core::_do_background_thread(void* ctx)
{
	trunk_zmq_core* obj = (trunk_zmq_core*)ctx;
	obj->do_background_thread();
	return (NULL);
}

void trunk_zmq_core::do_background_thread()
{
	zmq::proxy(static_cast<void*>(d_clients), static_cast<void*>(d_workers), nullptr);
}

bool trunk_zmq_core::start()
{
	if (is_running()) return false;

	pthread_create(&d_background_thread, NULL, _do_background_thread, (void*)this);

	return true;
}

void trunk_zmq_core::stop()
{
	if (!is_running()) return;

	//need to stop d_background_thread
	d_background_thread = 0;
}

bool trunk_zmq_core::is_running()
{
	return d_background_thread != 0;
}

bool trunk_zmq_core::register_worker(trunk_zmq_worker* worker)
{
	if (worker == NULL) return false;

	return worker->connect_worker(get_context());
}