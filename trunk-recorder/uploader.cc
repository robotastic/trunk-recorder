#include "uploader.h"

using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
typedef ssl::stream<tcp::socket>      ssl_socket;
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
    BOOST_LOG_TRIVIAL(info)  << "Verifying: " << subject_name << ""  "Verified: " << verified << std::endl;
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

void build_call_request(struct call_data_t *call, boost::asio::streambuf& request_) {
  // boost::asio::streambuf request_;
  // std::string server = "api.openmhz.com";
  // std::string path =  "/upload";
  std::string boundary("MD5_0be63cda3bf42193e4303db2c5ac3138");


  std::string form_name("call");
  std::string form_filename(call->converted);

  std::ifstream file(call->converted, std::ios::binary);

  // Make sure we have something to read.
  if (!file.is_open()) {
    BOOST_LOG_TRIVIAL(info) << "Error opening file ";

    // throw (std::exception("Could not open file."));
  }

  // Copy contents "as efficiently as possible".


  // ------------------------------------------------------------------------
  // Create Disposition in a stringstream, because we need Content-Length...
  std::ostringstream oss;
  oss << "--" << boundary << "\r\n";
  oss << "Content-Disposition: form-data; name=\"" << form_name << "\"; filename=\"" << form_filename << "\"\r\n";

  // oss << "Content-Type: text/plain\r\n";
  oss << "Content-Type: application/octet-stream\r\n";
  oss << "Content-Transfer-Encoding: binary\r\n";
  oss << "\r\n";

  oss << file.rdbuf();
  file.close();

  std::string source_list = "[";

  if (call->source_count != 0) {
    for (int i = 0; i < call->source_count; i++) {
      source_list = source_list + "{ \"pos\": " + boost::lexical_cast<std::string>(call->source_list[i].position) + ", \"src\": " + boost::lexical_cast<std::string>(call->source_list[i].source) + " }";

      if (i < (call->source_count - 1)) {
        source_list = source_list + ", ";
      } else {
        source_list = source_list + "]";
      }
    }
  } else {
    source_list = "[]";
  }

  std::string freq_list = "[";

  if (call->freq_count != 0) {
    for (int i = 0; i < call->freq_count; i++) {
      freq_list = freq_list + "{ \"pos\": " + boost::lexical_cast<std::string>(call->freq_list[i].position) + ", \"freq\": " + boost::lexical_cast<std::string>(call->freq_list[i].freq) + " }";

      if (i < (call->freq_count - 1)) {
        freq_list = freq_list + ", ";
      } else {
        freq_list = freq_list + "]";
      }
    }
  } else {
    freq_list = "[]";
  }

  add_post_field(oss, "freq",          boost::lexical_cast<std::string>(call->freq),       boundary);
  add_post_field(oss, "start_time",    boost::lexical_cast<std::string>(call->start_time), boundary);
  add_post_field(oss, "stop_time",     boost::lexical_cast<std::string>(call->stop_time),  boundary);

  add_post_field(oss, "talkgroup_num", boost::lexical_cast<std::string>(call->talkgroup),  boundary);
  add_post_field(oss, "emergency",     boost::lexical_cast<std::string>(call->emergency),  boundary);
  add_post_field(oss, "api_key",       call->api_key,                                      boundary);
  add_post_field(oss, "source_list",   source_list,                                        boundary);
  add_post_field(oss, "freq_list",     freq_list,                                          boundary);

  oss << "\r\n--" << boundary << "--\r\n";
  const std::string& body_str(oss.str());

  // oss.clear();
  // oss.flush();
  // ------------------------------------------------------------------------


  std::ostream post_stream(&request_);

  post_stream << "POST " << call->path << "" << " HTTP/1.1\r\n";
  post_stream << "Content-Type: multipart/form-data; boundary=" << boundary << "\r\n";
  post_stream << "User-Agent: TrunkRecorder1.0\r\n";
  post_stream << "Host: " << call->hostname << "\r\n"; // The domain name of the
                                                       // server (for virtual
                                                       // hosting), mandatory
                                                       // since HTTP/1.1
  post_stream << "Accept: */*\r\n";
  post_stream << "Connection: Close\r\n";
  post_stream << "Cache-Control: no-cache\r\n";
  post_stream << "Content-Length: " << body_str.size() << "\r\n"; // size_of_stream(oss)
                                                                  // << "\r\n";
  post_stream << "\r\n";

  post_stream << body_str;
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
      BOOST_LOG_TRIVIAL(info) << "SSL: Response returned with status code " << status_code << "";
      return 1;
    }

    // std::cout << "Response code: " << status_code << "\n";

    // Read the response headers, which are terminated by a blank line.
    boost::asio::read_until(socket, response, "\r\n\r\n");

    // Process the response headers.
    std::string header;

    while (std::getline(response_stream, header) && header != "\r") {
      BOOST_LOG_TRIVIAL(info) << header << "";
    }
    BOOST_LOG_TRIVIAL(info) << "";


    // Write whatever content we already have to output.
    if (response.size() > 0) {
      BOOST_LOG_TRIVIAL(info) << &response;
    }

    // Read until EOF, writing data to output as we go.
    boost::system::error_code error;

    while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error)) {
      BOOST_LOG_TRIVIAL(info) << &response;
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

    // std::cout << "Resolve OK" << "\n";
    socket.set_verify_mode(boost::asio::ssl::verify_peer);

    // socket.set_verify_callback(boost::bind(&client::verify_certificate, this,
    // _1, _2));
    // socket.set_verify_callback(ssl::rfc2818_verification(server));
    socket.set_verify_callback(make_verbose_verification(boost::asio::ssl::rfc2818_verification(server_info->hostname)));
    boost::asio::connect(socket.lowest_layer(), endpoint_iterator, ec);

    if (ec)
    {
      BOOST_LOG_TRIVIAL(info) << "SSL: Connect failed: " << ec.message() << "";
      return 1;
    }

    // std::cout << "Connect OK " << "\n";
    socket.handshake(boost::asio::ssl::stream_base::client, ec);

    if (ec)
    {
      BOOST_LOG_TRIVIAL(info) << "SSL: Handshake failed: " << ec.message() << "";
      return 1;
    }

    // std::cout << "Request: " << "\n";
    const char *req_header = boost::asio::buffer_cast<const char *>(request_.data());

    // std::cout << req_header << "\n";


    // The handshake was successful. Send the request.

    boost::asio::write(socket, request_, ec);

    if (ec)
    {
      BOOST_LOG_TRIVIAL(info) << "SSL: Error write req: " << ec.message() << "";
      return 1;
    }

    BOOST_LOG_TRIVIAL(info) << &request_;

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
      BOOST_LOG_TRIVIAL(info) << "SSL: Response returned with status code " << status_code << "";
      return 1;
    }

    // Read the response headers, which are terminated by a blank line.
    boost::asio::read_until(socket, response, "\r\n\r\n", ec);

    // Process the response headers.
    std::string header;

    while (std::getline(response_stream, header) && header != "\r") {
      BOOST_LOG_TRIVIAL(info) <<header << "";
    }
    BOOST_LOG_TRIVIAL(info) << "";

    // Write whatever content we already have to output.
    if (response.size() > 0) {
      BOOST_LOG_TRIVIAL(info) << &response;
    }
    // Read until EOF, writing data to output as we go.
    boost::system::error_code error;

    while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error)) {
      BOOST_LOG_TRIVIAL(info) << &response;
    }
    if (error != boost::asio::error::eof) throw boost::system::system_error(error);
  }
  catch (std::exception& e)
  {
    BOOST_LOG_TRIVIAL(info) << "SSL Exception: " << e.what() << "";


    return 1;
  }
  return 0;
}

void convert_upload_call(call_data_t *call_info, server_data_t *server_info) {
  char shell_command[400];

  sprintf(shell_command, "ffmpeg -y -i %s  -c:a libfdk_aac -b:a 32k -cutoff 18000 -hide_banner -loglevel panic %s ", call_info->filename, call_info->converted);

  // BOOST_LOG_TRIVIAL(info) << "Converting: " << call_info->converted << "\n";
  // BOOST_LOG_TRIVIAL(info) <<"Command: " << shell_command << "\n";
  int rc = system(shell_command);

  // BOOST_LOG_TRIVIAL(info) << "Finished converting\n";

  boost::asio::streambuf request_;

  build_call_request(call_info, request_);

  // BOOST_LOG_TRIVIAL(info) << "Finished Build Call Request\n";

  size_t req_size = request_.size();

  if (call_info->scheme == "http") {
    BOOST_LOG_TRIVIAL(info) << "HTTP Upload result: " << http_upload(server_info, request_);
  }

  if (call_info->scheme == "https") {
    BOOST_LOG_TRIVIAL(info) << "HTTPS Upload result: " << https_upload(server_info, request_);
  }

  // BOOST_LOG_TRIVIAL(info) << "Try to clear: " << req_size;
  request_.consume(req_size);
}

void* upload_thread(void *thread_arg) {
  call_data_t   *call_info;
  server_data_t *server_info = new server_data_t;

  pthread_detach(pthread_self());

  call_info                  = static_cast<call_data_t *>(thread_arg);
  server_info->upload_server = call_info->upload_server;
  server_info->scheme        = call_info->scheme;
  server_info->hostname      = call_info->hostname;
  server_info->port          = call_info->port;
  server_info->path          = call_info->path;

  boost::filesystem::path m4a(call_info->filename);
  m4a = m4a.replace_extension(".m4a");
  const std::string& m4a_str(m4a.string());
  strcpy(call_info->converted, m4a_str.c_str());

  convert_upload_call(call_info, server_info);

  delete(server_info);
  delete(call_info);
  return NULL;

  // pthread_exit(NULL);
}

void send_call(Call *call, System *sys, Config config) {
  // struct call_data_t *call_info = (struct call_data_t *) malloc(sizeof(struct
  // call_data_t));
  call_data_t *call_info = new call_data_t;
  pthread_t    thread;


  boost::regex  ex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
  boost::cmatch what;

  if (regex_match(config.upload_server.c_str(), what, ex))
  {
    // from: http://www.zedwood.com/article/cpp-boost-url-regex
    call_info->upload_server = config.upload_server;
    call_info->scheme        = std::string(what[1].first, what[1].second);
    call_info->hostname      = std::string(what[2].first, what[2].second);
    call_info->port          = std::string(what[3].first, what[3].second);
    call_info->path          = std::string(what[4].first, what[4].second);

    // std::cout << "Upload - Scheme: " << call_info->scheme << " Hostname: " <<
    // call_info->hostname << " Port: " << call_info->port << " Path: " <<
    // call_info->path << "\n";
    strcpy(call_info->filename, call->get_filename());
  } else {
    // std::cout << "Unable to parse Server URL\n";
    return;
  }

  // std::cout << "Setting up thread\n";
  Call_Source *source_list = call->get_source_list();
  Call_Freq   *freq_list   = call->get_freq_list();
  call_info->talkgroup    = call->get_talkgroup();
  call_info->freq         = call->get_freq();
  call_info->encrypted    = call->get_encrypted();
  call_info->emergency    = call->get_emergency();
  call_info->tdma         = call->get_tdma();
  call_info->source_count = call->get_source_count();
  call_info->freq_count   = call->get_freq_count();
  call_info->start_time   = call->get_start_time();
  call_info->stop_time    = call->get_stop_time();
  call_info->api_key      = sys->get_api_key();
  call_info->short_name   = sys->get_short_name();
  std::stringstream ss;
  ss << "/" << sys->get_short_name() << "/upload";
  call_info->path = ss.str();

  // std::cout << "Upload - Scheme: " << call_info->scheme << " Hostname: " <<
  // call_info->hostname << " Port: " << call_info->port << " Path: " <<
  // call_info->path << "\n";

  for (int i = 0; i < call_info->source_count; i++) {
    call_info->source_list[i] = source_list[i];
  }

  for (int i = 0; i < call_info->freq_count; i++) {
    call_info->freq_list[i] = freq_list[i];
  }

  BOOST_LOG_TRIVIAL(info) << "Creating Upload Thread";
  int rc = pthread_create(&thread, NULL, upload_thread, (void *)call_info);

  // pthread_detach(thread);

  if (rc) {
    printf("ERROR; return code from pthread_create() is %d", rc);
  }
}
