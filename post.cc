/*******************************************************************************
#      ____               __          __  _      _____ _       _               #
#     / __ \              \ \        / / | |    / ____| |     | |              #
#    | |  | |_ __   ___ _ __ \  /\  / /__| |__ | |  __| | ___ | |__   ___      #
#    | |  | | '_ \ / _ \ '_ \ \/  \/ / _ \ '_ \| | |_ | |/ _ \| '_ \ / _ \     #
#    | |__| | |_) |  __/ | | \  /\  /  __/ |_) | |__| | | (_) | |_) |  __/     #
#     \____/| .__/ \___|_| |_|\/  \/ \___|_.__/ \_____|_|\___/|_.__/ \___|     #
#           | |                                                                #
#           |_|                                                                #
#                                                                              #
#                                (c) 2011 by                                   #
#           University of Applied Sciences Northwestern Switzerland            #
#                     Institute of Geomatics Engineering                       #
#                           martin.christen@fhnw.ch                            #
********************************************************************************
*     Licensed under MIT License. Read the file LICENSE for more information   *
*******************************************************************************/

#include "Post.h"


#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

using boost::asio::ip::tcp;

void HttpPost::ParseUrl(const std::string& in_url, std::string& out_host, std::string& out_fileName)
{
	int n = in_url.find("http://");
   std::string url;
	if(n != std::string::npos)
	{
		url = in_url.substr(n + std::string("http://").length(), -1);
	}
	n = url.find( "/" );
	out_host = url.substr(0, n);
	out_fileName = url.substr(n, -1);
}

unsigned int HttpPost::SendBinary(const std::string& url, std::string& form_name, std::string& form_filename, unsigned char* pData, size_t size)
{
   // Implementation according to RFC 2388 (http://www.ietf.org/rfc/rfc2388.txt)
   std::string host;
   std::string filename;
   unsigned int status_code = 0;

   FilenameUtils::ParseUrl(url, host, filename);

   try
	{
      boost::asio::io_service io_service;
      tcp::resolver resolver(io_service);
      tcp::resolver::query query( host, "http" );
      tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
      tcp::resolver::iterator end;
      tcp::socket socket(io_service);
      boost::system::error_code error = boost::asio::error::host_not_found;

      while (error && endpoint_iterator != end)
		{
			socket.close();
			socket.connect(*endpoint_iterator++, error);
		}

		if (error)
		{
      	throw boost::system::system_error(error);
      }

      std::string boundary("MD5_0be63cda3bf42193e4303db2c5ac3138");


      //------------------------------------------------------------------------
      // Create Disposition in a stringstream, because we need Content-Length...
      std::ostringstream oss;
      oss << "--" << boundary << "\r\n";
      oss << "Content-Disposition: form-data; name=\"" << form_name << "\"; filename=\"" << form_filename << "\"\r\n";
      //oss << "Content-Type: text/plain\r\n";
      oss << "Content-Type: application/octet-stream\r\n";
      oss << "Content-Transfer-Encoding: binary\r\n";
      oss << "\r\n";
      for (size_t i=0;i<size;i++)
      {
         oss << pData[i];
      }
      oss << "\r\n--" << boundary << "--\r\n";
      //------------------------------------------------------------------------

      boost::asio::streambuf post;
		std::ostream post_stream(&post);
		post_stream << "POST " << filename << "" << " HTTP/1.1\r\n";
      post_stream << "Content-Type: multipart/form-data; boundary=" << boundary << "\r\n";
		post_stream << "User-Agent: OpenWebGlobe/1.0\r\n";
      post_stream << "Host: " << host << "\r\n";   // The domain name of the server (for virtual hosting), mandatory since HTTP/1.1
      post_stream << "Accept: */*\r\n";
      post_stream << "Connection: Close\r\n";
      post_stream << "Cache-Control: no-cache\r\n";
      post_stream << "Content-Length: " << oss.str().size() << "\r\n";
      post_stream << "\r\n";

      post_stream << oss.str();

      int n = boost::asio::write( socket, post );

      // now receive header

      boost::asio::streambuf header;
		std::istream header_stream(&header);
		boost::asio::read_until( socket, header, "\r\n" );

      status_code = 0;

      std::string http_version;
		std::string status_message;
		std::string headBuff;
		header_stream >> http_version;
		header_stream >> status_code;
      std::getline(header_stream, status_message);

      if( status_code/100 != 2 || http_version.substr( 0, 5 ) != "HTTP/" )
		{
			throw boost::system::system_error( error, (boost::lexical_cast<std::string>(status_code)).c_str() );
		}

		while( true )
		{
			boost::asio::read( socket, header, boost::asio::transfer_at_least(1), error );
			if(header.size() == 0)
			{
				break;
			}

         std::cout << &header;

		}

		if (error != boost::asio::error::eof)
		{
			throw boost::system::system_error(error);
		}
		socket.close();


   }
   catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << __LINE__ << "\n";
		return 0;
	}

   return 0;
}


int main(void)
{
std::string url("http://api.openmhz.com/upload");
   std::string form_name("pic");
   std::string form_filename("myfile.bla");
   unsigned char* data = (unsigned char*)malloc(1024);
   memset(data,66,1024);
   HttpPost::SendBinary(url, form_name, form_filename, data, 1024);

}
