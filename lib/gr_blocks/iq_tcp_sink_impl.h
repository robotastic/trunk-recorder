#ifndef _iq_tcp_sink_IMPL_H_
#define _iq_tcp_sink_IMPL_H_

#include <string>

#include <boost/log/trivial.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/thread/thread.hpp>
#include <gnuradio/blocks/api.h>
#include <gnuradio/sync_block.h>

#include "iq_tcp_sink.h"

namespace gr {
	namespace blocks {

		class iq_tcp_sink_impl :public iq_tcp_sink {
		private:
			ISourceIntf* d_source;
			boost::system::error_code ec;
			boost::asio::io_service d_io_service;
			boost::asio::ip::tcp::socket *tcpsocket = NULL;
			boost::asio::ip::tcp::acceptor *d_acceptor = NULL;

			boost::mutex d_mutex;

			boost::thread* listener_thread;
			bool thread_running;
			bool stop_thread;
			bool start_new_listener;
			bool initial_connection;

			std::string d_host;
			int d_port;
			bool d_connected;
			
		protected:

			virtual void checkForDisconnect();
			virtual void connect(bool initialConnection);
			virtual void run_listener();

			virtual void processCommands();

		public:

			typedef boost::shared_ptr<iq_tcp_sink_impl> sptr;

			static sptr make(ISourceIntf *source, const std::string &host, int port);

			iq_tcp_sink_impl(ISourceIntf *source, const std::string& host, int port);
			~iq_tcp_sink_impl();

			int work(int noutput_items,
				gr_vector_const_void_star& input_items,
				gr_vector_void_star& output_items);

			void accept_handler(boost::asio::ip::tcp::socket* new_connection, const boost::system::error_code& error);

			virtual bool stop();
		};
	}
}

#endif // _iq_tcp_sink_IMPL_H_