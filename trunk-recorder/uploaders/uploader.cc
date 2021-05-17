#include "uploader.h"

using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;

typedef boost::shared_ptr<tcp::socket>tcp_socket_ptr;

///@brief Helper class that prints the current certificate's subject
///       name and the verification results.
template<typename Verifier>
class verbose_verification {
public:

  verbose_verification(Verifier verifier)
    : verifier_(verifier)
  {}

  bool operator()(
    bool                              preverified,
    boost::asio::ssl::verify_context& ctx
    )
  {
    char  subject_name[256];
    X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());

    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    bool verified = verifier_(preverified, ctx);
    BOOST_LOG_TRIVIAL(trace)  << "Verifying: " << subject_name << ""  "Verified: " << verified << std::endl;
    return verified;
  }

private:

  Verifier verifier_;
};

///@brief Auxiliary function to make verbose_verification objects.
template<typename Verifier>
verbose_verification<Verifier>

make_verbose_verification(Verifier verifier)
{
  return verbose_verification<Verifier>(verifier);
}

void add_post_field(std::ostringstream& post_stream, std::string name, std::string value, std::string boundary) {
  post_stream << "\r\n--" << boundary << "\r\n";
  post_stream << "Content-Disposition: form-data; name=\"" << name << "\"\r\n";
  post_stream << "\r\n";
  post_stream << value;
}

int http_upload(struct server_data_t *server_info,   boost::asio::streambuf& request_)
{
  try
  {
    boost::asio::io_service   io_service;
    boost::system::error_code ec;

    // Get a list of endpoints corresponding to the server name.
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(server_info->hostname, server_info->port, tcp::resolver::query::canonical_name);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    // Try each endpoint until we successfully establish a connection.
    boost::asio::ip::tcp::socket socket(io_service);

    // tcp_socket_ptr socket = new socket(io_service);
    boost::asio::connect(socket, endpoint_iterator);

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.


    // Send the request.
    boost::asio::write(socket, request_);

    // Read the response status line. The response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    boost::asio::streambuf response;
    boost::asio::read_until(socket, response, "\r\n");

    // Check that response is OK.
    std::istream response_stream(&response);
    std::string  http_version;
    response_stream >> http_version;
    unsigned int status_code;
    response_stream >> status_code;
    std::string status_message;
    std::getline(response_stream, status_message);

    if (!response_stream || (http_version.substr(0, 5) != "HTTP/"))
    {
      BOOST_LOG_TRIVIAL(info) << "Invalid response";
      return 1;
    }

    if (status_code != 200)
    {
      BOOST_LOG_TRIVIAL(info) << "Response returned with status code " << status_code << "";
      return 1;
    }

    // std::cout << "Response code: " << status_code << "\n";

    // Read the response headers, which are terminated by a blank line.
    boost::asio::read_until(socket, response, "\r\n\r\n");

    // Process the response headers.
    std::string header;

    while (std::getline(response_stream, header) && header != "\r") {
      BOOST_LOG_TRIVIAL(trace) << header << "";
    }
    BOOST_LOG_TRIVIAL(trace) << "";


    // Write whatever content we already have to output.
    if (response.size() > 0) {
      BOOST_LOG_TRIVIAL(trace) << &response;
    }

    // Read until EOF, writing data to output as we go.
    boost::system::error_code error;

    while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error)) {
      BOOST_LOG_TRIVIAL(trace) << &response;
    }

    if (error != boost::asio::error::eof) throw boost::system::system_error(error);

    socket.close(ec);

    if (ec)
    {
      BOOST_LOG_TRIVIAL(info) << "Error closing socket: " << ec << "";
    }
  }
  catch (std::exception& e)
  {
    BOOST_LOG_TRIVIAL(info) << "Socket: Exception: " << e.what() << "";
  }
  return 0;
}

int https_upload(struct server_data_t *server_info, boost::asio::streambuf& request_)
{
  // std::string server = "api.openmhz.com";
  // std::string path =  "/upload";

  typedef ssl::stream<tcp::socket>      ssl_socket;
  boost::asio::io_service io_service;
  boost::asio::streambuf  response_;

  // Open the file for the shortest time possible.


  // Create a context that uses the default paths for
  // finding CA certificates.
  ssl::context ctx(ssl::context::sslv23);

  ctx.set_default_verify_paths();
  boost::system::error_code ec;

  // Get a list of endpoints corresponding to the server name.
  tcp::resolver resolver(io_service);

  tcp::resolver::query query(server_info->hostname, "https");
  tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec);
  ssl_socket socket(io_service, ctx);
  try
  {
    if (ec)
    {
      BOOST_LOG_TRIVIAL(info) << "SSL: Error resolve: " << ec.message() << "";
      return 1;
    }

     //std::cout << "Resolve OK" << "\n";
    socket.set_verify_mode(boost::asio::ssl::verify_peer);

    // socket.set_verify_callback(boost::bind(&client::verify_certificate, this,
    // _1, _2));
    // socket.set_verify_callback(ssl::rfc2818_verification(server));
    socket.set_verify_callback(make_verbose_verification(boost::asio::ssl::rfc2818_verification(server_info->hostname)));
    boost::asio::connect(socket.lowest_layer(), endpoint_iterator, ec);

    if (ec)
    {
      BOOST_LOG_TRIVIAL(error) << "SSL: Connect failed: " << ec.message() << "";
      return 1;
    }

     //std::cout << "Connect OK " << "\n";
    socket.handshake(boost::asio::ssl::stream_base::client, ec);

    if (ec)
    {
      BOOST_LOG_TRIVIAL(error) << "SSL: Handshake failed: " << ec.message() << "";
      return 1;
    }

     //std::cout << "Request: " << "\n";
    //const char *req_header = boost::asio::buffer_cast<const char *>(request_.data());

     //std::cout << req_header << "\n";


    // The handshake was successful. Send the request.

    boost::asio::write(socket, request_, ec);

    if (ec)
    {
      BOOST_LOG_TRIVIAL(error) << "SSL: Error write req: " << ec.message() << "";
      return 1;
    }

    BOOST_LOG_TRIVIAL(trace) << &request_;

    // Read the response status line. The response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    boost::asio::streambuf response;

    boost::asio::read_until(socket, response, "\r\n", ec);

    // Check that response is OK.
    std::istream response_stream(&response);
    std::string  http_version;
    response_stream >> http_version;
    unsigned int status_code;
    response_stream >> status_code;
    std::string status_message;
    std::getline(response_stream, status_message);

    if (!response_stream || (http_version.substr(0, 5) != "HTTP/"))
    {
      BOOST_LOG_TRIVIAL(info) << "SSL: Invalid response";
      return 1;
    }

    if (status_code != 200)
    {
      BOOST_LOG_TRIVIAL(info) << "HTTPS: Response returned with status code " << status_code << "";
      return 1;
    }

    // Read the response headers, which are terminated by a blank line.
    boost::asio::read_until(socket, response, "\r\n\r\n", ec);

    // Process the response headers.
    std::string header;

    while (std::getline(response_stream, header) && header != "\r") {
      BOOST_LOG_TRIVIAL(trace) <<"Header: " <<header << "";
    }
    BOOST_LOG_TRIVIAL(trace) << "";

    // Write whatever content we already have to output.
    if (response.size() > 0) {
  //    BOOST_LOG_TRIVIAL(trace) << "Response1: " <<&response;

    // Read until EOF, writing data to output as we go.
    boost::system::error_code error;

    while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error)) {
      BOOST_LOG_TRIVIAL(info) << "Response2: " << &response;
    }
    if (error != boost::asio::error::eof) throw boost::system::system_error(error);
    }
  }
  catch (std::exception& e)
  {
    BOOST_LOG_TRIVIAL(info) << "SSL Exception: " << e.what() << "";


    return 1;
  }
  return 0;
}
