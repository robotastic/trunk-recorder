#include "trunk_zmq_worker.h"
#include "trunk_zmq_core.h"
#include "zhelpers.h"

trunk_zmq_worker::trunk_zmq_worker()
	: d_zmq_running(false)
{
	//
}

trunk_zmq_worker::~trunk_zmq_worker()
{
	zmq_close(d_worker);
}

bool trunk_zmq_worker::connect_worker(zmq::context_t& context)
{
	if (d_zmq_running) return true;

	try {
		// Worker socket, publishes to the trunk_zmq_core::d_workers
		d_worker = new zmq::socket_t(context, ZMQ_PUB);
		d_worker->bind(trunk_zmq_core::INPROC_ADDR);

		d_zmq_running = true;
	}
	catch (std::exception const& e)
	{
		BOOST_LOG_TRIVIAL(error) << "ZMQ_WORKER: ERROR CONNECTING: " << e.what();
		d_zmq_running = false;
	}
	return d_zmq_running;
}

int trunk_zmq_worker::broadcast_message(const char* msg)
{
	if (!d_zmq_running) return 0;

	return s_send(d_worker, msg);
}