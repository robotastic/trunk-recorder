#include <gnuradio/io_signature.h>
#include <gnuradio/gr_complex.h>

#include "iq_tcp_sink.h"
#include "iq_tcp_sink_impl.h"

namespace gr {
	namespace blocks {

		iq_tcp_sink_impl::sptr
			iq_tcp_sink_impl::make(ISourceIntf *source, const std::string& host, int port)
		{
			return gnuradio::get_initial_sptr
			(new iq_tcp_sink_impl(source, host, port));
		}

		iq_tcp_sink_impl::iq_tcp_sink_impl(ISourceIntf *source, const std::string& host, int port)
			: sync_block("iq_tcp_sink",
				io_signature::make(1, 1, sizeof(gr_complex)),
				io_signature::make(0, 0, sizeof(gr_complex)))
		{
			d_source = source;
			d_port = port;
			d_host = host;
			thread_running = false;
			stop_thread = false;
			start_new_listener = true;
			initial_connection = true;
			listener_thread = new boost::thread(boost::bind(&iq_tcp_sink_impl::run_listener, this));
		}

		iq_tcp_sink_impl::~iq_tcp_sink_impl()
		{
			stop();
		}

		int iq_tcp_sink_impl::work(int noutput_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items)
		{
			gr::thread::scoped_lock guard(d_mutex);

			if (!d_connected)
				return noutput_items;

			unsigned int noi = noutput_items * sizeof(gr_complex);
			int bytesWritten;
			int bytesRemaining = noi;

			ec.clear();

			char* pBuff;
			pBuff = (char*)input_items[0];

			while ((bytesRemaining > 0) && (!ec)) {
				bytesWritten = boost::asio::write(*tcpsocket, boost::asio::buffer((const void*)pBuff, bytesRemaining), ec);
				bytesRemaining -= bytesWritten;
				pBuff += bytesWritten;

				if (ec == boost::asio::error::connection_reset || ec == boost::asio::error::broken_pipe) {
					d_connected = false;
					bytesRemaining = 0;

					BOOST_LOG_TRIVIAL(info) << "iq_tcp_sink_impl: Client disconnected. Waiting for new connection.";
					start_new_listener = true;
				}
			}

			processCommands();

			return noutput_items;
		}

		void iq_tcp_sink_impl::run_listener() {
			thread_running = true;

			while (!stop_thread) {
				if (start_new_listener) {
					start_new_listener = false;
					connect(initial_connection);
					initial_connection = false;
				}
				else {
					usleep(10);
				}
			}

			thread_running = false;
		}

		void iq_tcp_sink_impl::accept_handler(boost::asio::ip::tcp::socket* new_connection, const boost::system::error_code& error) {
			if (!error) {
				BOOST_LOG_TRIVIAL(info) << "iq_tcp_sink: Client connection received.";
				tcpsocket = new_connection;

				boost::asio::socket_base::keep_alive option(true);
				tcpsocket->set_option(option);
				d_connected = true;
			}
			else {
				BOOST_LOG_TRIVIAL(error) << "iq_tcp_sink: Error: " << error << " accepting boost TCP session.";
				
				delete new_connection;
				d_connected = false;
				tcpsocket = NULL;
			}
		}

		void iq_tcp_sink_impl::connect(bool initialConnection) {
			BOOST_LOG_TRIVIAL(info) << "iq_tcp_sink: Waiting for connection on port " << d_port;

			if (initialConnection) {
				d_acceptor = new boost::asio::ip::tcp::acceptor(d_io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), d_port));
			}
			else {
				d_io_service.reset();
			}

			if (tcpsocket) {
				delete tcpsocket;
			}
			tcpsocket = NULL;
			d_connected = false;

			boost::asio::ip::tcp::socket* tmpSocket = new boost::asio::ip::tcp::socket(d_io_service);
			d_acceptor->async_accept(*tmpSocket, boost::bind(&iq_tcp_sink_impl::accept_handler, this, tmpSocket, boost::asio::placeholders::error));

			d_io_service.run();
		}

		bool iq_tcp_sink_impl::stop() {
			if (thread_running) {
				stop_thread = true;
			}

			if (tcpsocket) {
				tcpsocket->close();
				delete tcpsocket;
				tcpsocket = NULL;
			}

			d_io_service.reset();
			d_io_service.stop();

			if (d_acceptor) {
				delete d_acceptor;
				d_acceptor = NULL;
			}

			if (listener_thread) {
				while (thread_running)
					usleep(5);

				delete listener_thread;
				listener_thread = NULL;
			}

			return true;
		}

		///At this point, there isn't going to be much control over the Source just yet.
		void iq_tcp_sink_impl::processCommands()
		{
			if (tcpsocket->available() <= 0 || d_source == NULL) return;

			char buff[5];
			
			int bytesRead = tcpsocket->receive(boost::asio::buffer(buff), 0, ec);

			if (ec == boost::asio::error::connection_reset || ec == boost::asio::error::broken_pipe) {
				d_connected = false;
				
				BOOST_LOG_TRIVIAL(info) << "iq_tcp_sink_impl.processCommands: Client disconnected. Waiting for new connection.";
				start_new_listener = true;
				return;
			}

			if (bytesRead >= 1)
			{
				BOOST_LOG_TRIVIAL(trace) << "Processing iq_tcp_sink cmd: " << std::hex << buff[0];

				switch (buff[0])
				{
				case 0x01: break; // Set Center Frequency
				case 0x02: break; // Set Sample Rate
				case 0x03: break; // Set Gain Mode
				case 0x04: break; // Set Tuner Gain
				case 0x05: break; // Set Frequency Correction
				case 0x06: break; // Set I/F Stage Gain
				case 0x07: break; // Set Test Mode
				case 0x08: break; // Set AGC Mode
				case 0x09: break; // Set Direct Sampling
				case 0x0a: break; // Set Offset Tuning
				case 0x0b: break; // Set RTL XTAL
				case 0x0c: break; // Set Tuner XTAL
				case 0x0d: break; // Set Tuner Gain by Index
				case 0x0e: break; // Set Bias Tee
				}
			}
		}

		void iq_tcp_sink_impl::checkForDisconnect() {
			int bytesRead;

			char buff[1];
			bytesRead = tcpsocket->receive(boost::asio::buffer(buff),
				tcpsocket->message_peek, ec);
			if ((boost::asio::error::eof == ec) ||
				(boost::asio::error::connection_reset == ec)) {
				BOOST_LOG_TRIVIAL(info) << "iq_tcp_sink: Disconnect detected on " << d_host << ":" << d_port;
				tcpsocket->close();
				delete tcpsocket;
				tcpsocket = NULL;
			}
			else {
				if (ec) {
					BOOST_LOG_TRIVIAL(info) << "iq_tcp_sink: Socket error " << ec << " detected.";
				}
				else {
					BOOST_LOG_TRIVIAL(trace) << "iq_tcp_sink: Socket read " << bytesRead << " bytes.";
				}
			}
		}
	}
}