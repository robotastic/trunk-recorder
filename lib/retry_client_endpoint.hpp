#ifndef RETRY_CLIENT_ENDPOINT_HPP
#define RETRY_CLIENT_ENDPOINT_HPP

#include <websocketpp/endpoint.hpp>
#include <websocketpp/common/thread.hpp>

#ifdef _WEBSOCKETPP_CPP11_THREAD_
    #include <atomic>
#else
    #include <boost/atomic.hpp>
#endif

namespace websocketpp
{

/// The type and function signature of a retry configure handler
/**
 * The configure handler is triggered before copying over all the connection
 * specific configurations to the new connection it creates, via the
 * get_connection(...) method
 *
 * The configure handler will be called as long as the config has it's retry
 * flag set to true and has not exceeded the max_attempts
 */
typedef lib::function<void(connection_hdl)> configure_handler;

namespace lib
{
#ifdef _WEBSOCKETPP_CPP11_THREAD_
	using std::atomic;
#else
	using boost::atomic;
#endif
}

class retry_data
{
public:
	retry_data()
		: m_handle_id(-1)
		, m_retry(false)
		, m_retry_delay(4000)
		, m_attempt_count(1)
		, m_max_attempts(0)
	{
	}

	void set_configure_handler(configure_handler h)
	{
		lib::lock_guard<lib::mutex> scoped_lock_guard(m_mutex);
		m_configure_con_handler = h;
	}

	configure_handler get_configure_handler()
	{
		lib::lock_guard<lib::mutex> scoped_lock_guard(m_mutex);
		return m_configure_con_handler;
	}

	bool is_configure_handler_set()
	{
		lib::lock_guard<lib::mutex> scoped_lock_guard(m_mutex);
		return bool(m_configure_con_handler);
	}

	/// An optional ID that the user can use to track the connecting thread
	lib::atomic<int> m_handle_id;

	/// A flag that is interpreted in our retry thread allowing the user to stop retrying
	lib::atomic<bool> m_retry;

	/// Amount of time (in milliseconds) between each retry attempt
	lib::atomic<unsigned int> m_retry_delay;

	/// Tracks the number of connection attempts for this connection. Reset to 1 upon successful connection
	lib::atomic<unsigned int> m_attempt_count;

	/// Stops attempting to connect after X attempts. 0 = Infinite.
	lib::atomic<unsigned int> m_max_attempts;
private:
	lib::mutex m_mutex;

	/// The functor used when a new connection is made in our retry thread
	configure_handler m_configure_con_handler;
};

/// Retry Config is a helper struct
/**
 * This config struct is the most simplistic helper that we provide. You aren't
 * required to use it, the only thing that's required is that connection_base
 * is inherited/typed from websocketpp::retry_data.
 */
template<typename custom_config>
struct retry_config : public custom_config
{
	typedef retry_data connection_base;
};

/// Retry Client Endpoint role based on the given config
/**
 *
 */
template <typename config>
class retry_client_endpoint : public endpoint<connection<config>,config> {
public:
    /// Type of this endpoint
    typedef retry_client_endpoint<config> type;

    /// Type of the endpoint concurrency component
    typedef typename config::concurrency_type concurrency_type;
    /// Type of the endpoint transport component
    typedef typename config::transport_type transport_type;

    /// Type of the connections this server will create
    typedef connection<config> connection_type;
    /// Type of a shared pointer to the connections this server will create
    typedef typename connection_type::ptr connection_ptr;

    /// Type of the connection transport component
    typedef typename transport_type::transport_con_type transport_con_type;
    /// Type of a shared pointer to the connection transport component
    typedef typename transport_con_type::ptr transport_con_ptr;

    /// Type of the endpoint component of this server
    typedef endpoint<connection_type,config> endpoint_type;

    explicit retry_client_endpoint()
		: endpoint_type(false)
    {
        endpoint_type::m_alog.write(log::alevel::devel, "retry_client_endpoint constructor");
    }

    ~retry_client_endpoint()
    {
	}

	/// These set methods will Hide our endpoint.hpp methods
	/// This is done because they aren't virtual and we cannot override


    /// Get a new connection (string version)
    /**
     * Creates and returns a pointer to a new connection to the given URI
     * suitable for passing to connect(connection_ptr). This overload allows
     * default construction of the uri_ptr from a standard string.
     *
     * @param [in] u URI to open the connection to as a string
     * @param [out] ec An status code indicating failure reasons, if any
     *
     * @return A connection_ptr to the new connection
     */
    connection_ptr get_connection(std::string const & u, lib::error_code & ec) {
        uri_ptr location(new uri(u));

        if (!location->get_valid()) {
            ec = error::make_error_code(error::invalid_uri);
            return connection_ptr();
        }

        return get_connection(location, ec);
    }

    /// Get a new connection
    /**
     * Creates and returns a pointer to a new connection to the given URI
     * suitable for passing to connect(connection_ptr). This method allows
     * applying connection specific settings before performing the opening
     * handshake.
     *
     * @param [in] location URI to open the connection to as a uri_ptr
     * @param [out] ec An status code indicating failure reasons, if any
     *
     * @return A connection_ptr to the new connection
     */
    connection_ptr get_connection(uri_ptr location, lib::error_code & ec) {
        if (location->get_secure() && !transport_type::is_secure()) {
            ec = error::make_error_code(error::endpoint_not_secure);
            return connection_ptr();
        }

        connection_ptr con = endpoint_type::create_connection();

        if (!con) {
            ec = error::make_error_code(error::con_creation_failed);
            return con;
        }

        con->set_uri(location);

        ec = lib::error_code();
        return con;
    }

    /// Begin the connection process for the given connection
    /**
     * Initiates the opening connection handshake for connection con. Exact
     * behavior depends on the underlying transport policy.
     *
     * @param con The connection to connect
     */
    void connect(connection_ptr con) {
		if(con->is_configure_handler_set())
		{
			// We save and overwrite the m_retry_count, because the user could potentially change it, and we don't want this.
			int attempt_count = con->m_attempt_count;
			con->get_configure_handler()(con);
			con->m_attempt_count = attempt_count;
		}
		else if(con->m_retry == true)
		{
			// We require the user to set a configure connection handler if retry is == true.
			// They must set it (even if empty because they have to be aware that it exists
			// and that they don't want to configure anything additional
			throw websocketpp::exception("retry_client_endpoint.hpp: no configure_handler has been set");
		}

		transport_type::async_connect(
            lib::static_pointer_cast<transport_con_type>(con),
            con->get_uri(),
            lib::bind(
                &type::handle_connect,
                this,
                con,
                lib::placeholders::_1
            )
        );
    }

    /// Initiates a retry on close
    /**
     * The user uses this method as a helper when they want to attempt to connect on a
     * connection close. They call this method, typically from the on_close handler,
     * passing in the old ptr, and a fresh new connection ptr that they have gotten
     * from the get_connection method. This method will reset the retry_count, but all
     * other settings remain the same
     *
     * @param
     */
    void connect(connection_ptr old_con, connection_ptr new_con)
    {
		if(attempt_retry(old_con))
		{
			swap_and_configure_retry_settings(old_con, new_con);
			connect(new_con);
		}
		else
		{
			throw websocketpp::exception("retry_client_endpoint.hpp: old_connection settings indicate to NOT attempt a retry");
		}
	}

private:
	void swap_and_configure_retry_settings(connection_ptr old_con, connection_ptr new_con)
	{
		// copy over any of the connection settings before we call the client's configure_handler
		// Because in the configure_handler, the client has the ability to overwrite
		// any of the old settings, because they  might want the criteria to change
		new_con->m_handle_id = old_con->m_handle_id.load();
		new_con->m_retry = old_con->m_retry.load();
		new_con->m_retry_delay = old_con->m_retry_delay.load();
		new_con->m_attempt_count = old_con->m_attempt_count.load();
		new_con->m_max_attempts = old_con->m_max_attempts.load();
		new_con->set_configure_handler(old_con->get_configure_handler());
	}

	bool attempt_retry(connection_ptr con)
	{
		return con->m_retry == true && (con->m_max_attempts == 0 || con->m_attempt_count < con->m_max_attempts);
	}

    void handle_connect(connection_ptr con, lib::error_code const & ec) {
        if (ec) {
            con->terminate(ec);

            endpoint_type::m_elog.write(log::elevel::rerror,
                    "handle_connect error: "+ec.message());

            // Now we try to connect again if configured that way.
            if(attempt_retry(con))
            {
				// We wait the amont of delay before attempting to retry
#ifdef _WEBSOCKETPP_CPP11_THREAD_
				std::this_thread::sleep_for(std::chrono::milliseconds(con->m_retry_delay));
#else
				boost::this_thread::sleep_for(boost::chrono::milliseconds(con->m_retry_delay));
#endif

				lib::error_code thread_ec;
				connection_ptr new_con = this->get_connection(con->get_uri(), thread_ec);

				if(thread_ec)
				{
					endpoint_type::m_elog.write(log::elevel::rerror,
                    "handle_connect trying to retry get_connection error: "+thread_ec.message());
				}
				else
				{
					swap_and_configure_retry_settings(con, new_con);

					if(attempt_retry(new_con))
					{
						// Increment our attempt count
						++new_con->m_attempt_count;
						// Try to connect again
						connect(new_con); // We don't care about returning the conPtr
					}
					else
					{
						// Stop, as the user changed the settings in their m_configure_con_handler
						endpoint_type::m_alog.write(log::elevel::devel, "handle_connect not attempting to connect after configure callback, because retry settings evaluated to false");
					}
				}
			}
			else
			{
				// TODO: Perhaps have a callback for when settings evaluate to false?
				endpoint_type::m_alog.write(log::elevel::devel, "handle_connect not attempting to connect because retry settings evaluated to false");
			}
        } else {
            endpoint_type::m_alog.write(log::alevel::connect,
                "Successful connection");

            con->m_attempt_count = 1;
            con->start();
        }
    }
};

} // namespace websocketpp

#endif // RETRY_CLIENT_ENDPOINT_HPP
