#ifndef DOWNLOAD_CPP
#define DOWNLOAD_CPP

#include "Util/download.h"
#include "Doppelganger/Core.h"
#include <string>
#include <fstream>

#include <boost/certify/extensions.hpp>
#include <boost/certify/https_verification.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace Doppelganger
{
	namespace Util
	{
		bool download(const std::shared_ptr<Core> &core, const std::string &targetUrl, const fs::path &destPath)
		{
			namespace beast = boost::beast; // from <boost/beast.hpp>
			namespace http = beast::http;	// from <boost/beast/http.hpp>
			namespace net = boost::asio;	// from <boost/asio.hpp>
			namespace ssl = net::ssl;		// from <boost/asio/ssl.hpp>
			using tcp = net::ip::tcp;		// from <boost/asio/ip/tcp.hpp>

			// https://www.boost.org/doc/libs/develop/libs/beast/example/http/client/sync/http_client_sync.cpp
			// https://www.boost.org/doc/libs/develop/libs/beast/example/http/client/sync-ssl/http_client_sync_ssl.cpp
			try
			{

				std::string url = std::move(targetUrl);

				std::string protocol;
				int findPos = url.find("https://");
				protocol = std::string((findPos == std::string::npos) ? "http://" : "https://");
				url = url.substr(protocol.size());

				std::string host;
				findPos = url.find("/");
				host = url.substr(0, findPos);
				url = url.substr(host.size());

				std::string port;
				findPos = host.find(":");
				if (findPos == std::string::npos)
				{
					port = std::string((protocol == "http://") ? "80" : "443");
				}
				else
				{
					port = host.substr(findPos + 1);
					host = host.substr(0, findPos);
				}

				std::string target = std::move(url);

				const int version = 11;

				// The io_context is required for all I/O
				net::io_context ioc;

				// The SSL context is required, and holds certificates
				ssl::context ctx(ssl::context::tlsv12_client);

#if 0
			// This holds the root certificate used for verification
			load_root_certificates(ctx);
			// Verify the remote server's certificate
			ctx.set_verify_mode(ssl::verify_peer);
#else
				// https://stackoverflow.com/questions/49507407/using-boost-beast-asio-http-client-with-ssl-https
				ctx.set_verify_mode(ssl::context::verify_peer);
				boost::certify::enable_native_https_server_verification(ctx);
#endif

				// These objects perform our I/O
				tcp::resolver resolver(ioc);
				beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

				// Set SNI Hostname (many hosts need this to handshake successfully)
				if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
				{
					beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
					throw beast::system_error{ec};
				}

				// Look up the domain name
				auto const results = resolver.resolve(host, port);

				// Make the connection on the IP address we get from a lookup
				beast::get_lowest_layer(stream).connect(results);

				// Perform the SSL handshake
				stream.handshake(ssl::stream_base::client);

				// Set up an HTTP GET request message
				http::request<http::string_body> req{http::verb::get, target, version};
				req.set(http::field::host, host);
				req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

				// Send the HTTP request to the remote host
				http::write(stream, req);

				// This buffer is used for reading and must be persisted
				beast::flat_buffer buffer;

				// Declare a container to hold the response
				http::response<http::dynamic_body> res;

				// Receive the HTTP response
				http::read(stream, buffer, res);

				// Write the message to standard out
				// std::cout << res << std::endl;

				// write to file
				std::ofstream destFile(destPath, std::ios::binary);
				const std::string content = boost::beast::buffers_to_string(res.body().data());
				destFile.write(content.c_str(), content.size());
				destFile.close();

				// Gracefully close the stream
				beast::error_code ec;
				stream.shutdown(ec);
				if (ec == net::error::eof || ec == ssl::error::stream_truncated)
				{
					// Rationale:
					// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
					ec = {};
				}
				if (ec)
				{
					throw beast::system_error{ec};
				}

				// If we get here then the connection is closed gracefully
				return true;
			}
			catch (std::exception const &e)
			{
				core->logger.log(std::string(e.what()), "ERROR");
				return false;
			}
		}
	}
} // namespace

#endif
