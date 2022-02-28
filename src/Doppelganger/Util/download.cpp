#ifndef DOWNLOAD_CPP
#define DOWNLOAD_CPP

#include "Doppelganger/Util/download.h"
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

namespace
{
	namespace beast = boost::beast;
	namespace asio = boost::asio;
	namespace ssl = asio::ssl;
	namespace http = boost::beast::http;
	using tcp = boost::asio::ip::tcp;

	tcp::resolver::results_type resolve(asio::io_context &ctx, std::string const &hostname)
	{
		tcp::resolver resolver{ctx};
		return resolver.resolve(hostname, "https");
	}

	tcp::socket connect(asio::io_context &ctx, std::string const &hostname)
	{
		tcp::socket socket{ctx};
		asio::connect(socket, resolve(ctx, hostname));
		return socket;
	}

	std::unique_ptr<ssl::stream<tcp::socket>> connect(asio::io_context &ctx,
													  ssl::context &ssl_ctx,
													  std::string const &hostname)
	{
		auto stream = boost::make_unique<ssl::stream<tcp::socket>>(
			connect(ctx, hostname), ssl_ctx);
		// tag::stream_setup_source[]
		boost::certify::set_server_hostname(*stream, hostname);
		boost::certify::sni_hostname(*stream, hostname);
		// end::stream_setup_source[]

		stream->handshake(ssl::stream_base::handshake_type::client);
		return stream;
	}

	http::response<http::string_body> get(ssl::stream<tcp::socket> &stream,
										  boost::string_view hostname,
										  boost::string_view uri)
	{
		http::request<http::empty_body> request;
		request.method(http::verb::get);
		request.target(uri);
		request.keep_alive(false);
		request.set(http::field::host, hostname);
		http::write(stream, request);

		beast::flat_buffer buffer;
		http::response_parser<http::string_body> parser;
		// we disable body limit (this is not the best way, but works.)
		parser.body_limit(boost::none);
		http::read(stream, buffer, parser);
		return parser.get();
	}
}

namespace Doppelganger
{
	namespace Util
	{
		bool download(const std::string &targetUrl, const fs::path &destPath)
		{
			namespace beast = boost::beast;
			namespace asio = boost::asio;
			namespace ssl = asio::ssl;
			namespace http = boost::beast::http;
			using tcp = boost::asio::ip::tcp;

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
				std::string target = url.substr(host.size());

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

				asio::io_context ctx;
				ssl::context ssl_ctx{ssl::context::tlsv12_client};

				ssl_ctx.set_verify_mode(ssl::context::verify_peer | ssl::context::verify_fail_if_no_peer_cert);
				ssl_ctx.set_default_verify_paths();
				// tag::ctx_setup_source[]
				boost::certify::enable_native_https_server_verification(ssl_ctx);
				// end::ctx_setup_source[]
				auto stream_ptr = connect(ctx, ssl_ctx, host);
				// we need to care too many redirects...
				auto response = get(*stream_ptr, host, target);
				if (response.result() == boost::beast::http::status::ok)
				{
					// write to file
					std::ofstream destFile(destPath.string(), std::ios::binary);
					destFile << response.body();
					destFile.close();

					boost::system::error_code ec;
					stream_ptr->shutdown(ec);
					stream_ptr->next_layer().close(ec);

					return true;
				}
				else if (boost::beast::http::to_status_class(response.result()) == boost::beast::http::status_class::redirection)
				{
					std::string redirectedUrl("");
					if (response.find("location") != response.end())
					{
						redirectedUrl = response.at("location").to_string();
					}
					else if (response.find("Location") != response.end())
					{
						redirectedUrl = response.at("Location").to_string();
					}

					if (!redirectedUrl.empty())
					{
						return download(redirectedUrl, destPath);
					}
				}

				// neither
				//   - 200
				//   - 3XX (redirection)
				return false;
			}
			catch (std::exception const &e)
			{
				return false;
			}
		}
	}
} // namespace

#endif
