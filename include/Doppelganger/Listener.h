#ifndef LISTENER_H
#define LISTENER_H

#if defined(_WIN64)
#ifdef DLL_EXPORT
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif
#elif defined(__APPLE__)
#define DECLSPEC
#elif defined(__linux__)
#define DECLSPEC
#endif

#if defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#elif defined(__APPLE__)
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#elif defined(__linux__)
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/make_unique.hpp>
#include <boost/optional.hpp>

#include "Doppelganger/Core.h"
#include "Doppelganger/SSLDetector.h"

namespace Doppelganger
{
	namespace beast = boost::beast;			// from <boost/beast.hpp>
	namespace http = beast::http;			// from <boost/beast/http.hpp>
	namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
	namespace net = boost::asio;			// from <boost/asio.hpp>
	namespace ssl = boost::asio::ssl;		// from <boost/asio/ssl.hpp>
	using tcp = boost::asio::ip::tcp;		// from <boost/asio/ip/tcp.hpp>

	class DECLSPEC Listener : public std::enable_shared_from_this<Listener>
	{
	private:
		const std::shared_ptr<Core> core_;
		net::io_context &ioc_;
		ssl::context &ctx_;

		void fail(boost::system::error_code ec, char const *what)
		{
			if (ec == net::ssl::error::stream_truncated)
			{
				return;
			}

			std::stringstream s;
			s << what << ": " << ec.message();
			core_->logger.log(s.str(), "ERROR");
		}

	public:
		tcp::acceptor acceptor_;

		Listener(
			const std::shared_ptr<Core> &core,
			net::io_context &ioc,
			ssl::context &ctx,
			tcp::endpoint &endpoint)
			: core_(core), ioc_(ioc), ctx_(ctx), acceptor_(net::make_strand(ioc))
		{
			beast::error_code ec;

			// Open the acceptor
			acceptor_.open(endpoint.protocol(), ec);
			if (ec)
			{
				fail(ec, "open (Listener)");
				return;
			}

			// Allow address reuse
			acceptor_.set_option(net::socket_base::reuse_address(true), ec);
			if (ec)
			{
				fail(ec, "set_option (Listener)");
				return;
			}

			// Bind to the server address
			acceptor_.bind(endpoint, ec);
			if (ec)
			{
				fail(ec, "bind (Listener)");
				return;
			}

			// Start listening for connections
			acceptor_.listen(
				net::socket_base::max_listen_connections, ec);
			if (ec)
			{
				fail(ec, "listen (Listener)");
				return;
			}
		}

		// Start accepting incoming connections
		void run()
		{
			doAccept();
		}

	private:
		void doAccept()
		{
			// The new connection gets its own strand
			acceptor_.async_accept(
				net::make_strand(ioc_),
				beast::bind_front_handler(
					&Listener::onAccept,
					shared_from_this()));
		}

		void onAccept(beast::error_code ec, tcp::socket socket)
		{
			if (ec)
			{
				fail(ec, "accept (Listener)");
			}
			else
			{
				// Create the detector http_session and run it
				std::make_shared<SSLDetector>(
					core_,
					std::move(socket),
					ctx_)
					->run();
			}

			// Accept another connection
			doAccept();
		}
	};
}

#endif
