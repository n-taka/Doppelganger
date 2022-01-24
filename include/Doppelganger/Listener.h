#ifndef LISTENER_H
#define LISTENER_H

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <sstream>

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
#include "Doppelganger/Util/log.h"

namespace Doppelganger
{
	namespace beast = boost::beast;			// from <boost/beast.hpp>
	namespace http = beast::http;			// from <boost/beast/http.hpp>
	namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
	namespace net = boost::asio;			// from <boost/asio.hpp>
	namespace ssl = boost::asio::ssl;		// from <boost/asio/ssl.hpp>
	using tcp = boost::asio::ip::tcp;		// from <boost/asio/ip/tcp.hpp>

	class Listener : public std::enable_shared_from_this<Listener>
	{
	private:
		const std::weak_ptr<Core> core_;
		net::io_context &ioc_;
		ssl::context &ctx_;

		void fail(boost::system::error_code ec, char const *what)
		{
			if (ec == net::ssl::error::stream_truncated)
			{
				return;
			}

			{
				std::stringstream ss;
				ss << what << ": " << ec.message();
				Util::log(ss.str(), "ERROR", core_.lock()->dataDir_, core_.lock()->logConfig_.level, core_.lock()->logConfig_.type);
			}
		}

	public:
		tcp::acceptor acceptor_;

		Listener(
			const std::weak_ptr<Core> &core,
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
