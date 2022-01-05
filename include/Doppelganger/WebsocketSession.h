#ifndef WEBSOCKETSESSION_H
#define WEBSOCKETSESSION_H

// see
// https://www.boost.org/doc/libs/develop/libs/beast/example/advanced/server-flex/advanced_server_flex.cpp

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

#include <memory>
#include <string>
#include <sstream>
#include <vector>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>

#include "Doppelganger/Room.h"
#include "Doppelganger/Logger.h"

namespace Doppelganger
{
	namespace beast = boost::beast;			// from <boost/beast.hpp>
	namespace http = beast::http;			// from <boost/beast/http.hpp>
	namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
	namespace net = boost::asio;			// from <boost/asio.hpp>
	namespace ssl = boost::asio::ssl;		// from <boost/asio/ssl.hpp>
	using tcp = boost::asio::ip::tcp;		// from <boost/asio/ip/tcp.hpp>

	// Echoes back all received WebSocket messages.
	// This uses the Curiously Recurring Template Pattern so that
	// the same code works with both SSL streams and regular sockets.
	template <class Derived>
	class DECLSPEC WebsocketSession
	{
	private:
		// Access the derived class, this is part of
		// the Curiously Recurring Template Pattern idiom.
		Derived &derived();

		beast::flat_buffer buffer_;
		const std::shared_ptr<Room> room_;
		std::vector<std::shared_ptr<const std::string>> queue_;

		// Start the asynchronous operation
		template <class Body, class Allocator>
		void doAccept(http::request<Body, http::basic_fields<Allocator>> req)
		{
			// Set suggested timeout settings for the websocket
			derived().ws().set_option(
				websocket::stream_base::timeout::suggested(
					beast::role_type::server));

			// Set a decorator to change the Server of the handshake
			derived().ws().set_option(
				websocket::stream_base::decorator(
					[](websocket::response_type &res)
					{
						res.set(http::field::server,
								std::string(BOOST_BEAST_VERSION_STRING) +
									" Doppelganger");
					}));

			// Accept the websocket handshake
			derived().ws().async_accept(
				req,
				beast::bind_front_handler(
					&WebsocketSession::onAccept,
					derived().shared_from_this()));
		}

		void onAccept(beast::error_code ec);
		void doRead();
		void onRead(
			beast::error_code ec,
			std::size_t bytes_transferred);
		void doWrite();
		void onWrite(
			beast::error_code ec,
			std::size_t bytes_transferred);
		void fail(boost::system::error_code ec, char const *what);

	public:
		// constructor for initialization
		WebsocketSession(
			const std::shared_ptr<Room> &room,
			const std::string &UUID);

		// Start the asynchronous operation
		template <class Body, class Allocator>
		void run(http::request<Body, http::basic_fields<Allocator>> req)
		{
			// Accept the WebSocket upgrade request
			doAccept(std::move(req));
		}

		// Send a message
		void send(const std::shared_ptr<const std::string> &ss);

		const std::string UUID_;
	};

	//------------------------------------------------------------------------------

	// Handles a plain WebSocket connection
	class DECLSPEC PlainWebsocketSession
		: public WebsocketSession<PlainWebsocketSession>,
		  public std::enable_shared_from_this<PlainWebsocketSession>
	{
		websocket::stream<beast::tcp_stream> ws_;

	public:
		// Create the session
		explicit PlainWebsocketSession(
			const std::shared_ptr<Room> &room,
			const std::string &UUID,
			beast::tcp_stream &&stream);

		// Called by the base class
		websocket::stream<beast::tcp_stream> &ws();
	};

	//------------------------------------------------------------------------------

	// Handles an SSL WebSocket connection
	class DECLSPEC SSLWebsocketSession
		: public WebsocketSession<SSLWebsocketSession>,
		  public std::enable_shared_from_this<SSLWebsocketSession>
	{
		websocket::stream<
			beast::ssl_stream<beast::tcp_stream>>
			ws_;

	public:
		// Create the ssl_websocket_session
		explicit SSLWebsocketSession(
			const std::shared_ptr<Room> &room,
			const std::string &UUID,
			beast::ssl_stream<beast::tcp_stream> &&stream);

		// Called by the base class
		websocket::stream<beast::ssl_stream<beast::tcp_stream>> &ws();
	};

	//------------------------------------------------------------------------------

	// construct websocket for plain websocket
	template <class Body, class Allocator>
	void makeWebsocketSession(
		beast::tcp_stream stream,
		const std::shared_ptr<Room> &room,
		const std::string &UUID,
		http::request<Body, http::basic_fields<Allocator>> req)
	{
		std::make_shared<PlainWebsocketSession>(
			room,
			UUID,
			std::move(stream))
			->run(std::move(req));
	}

	// construct websocket for ssl websocket
	template <class Body, class Allocator>
	void makeWebsocketSession(
		beast::ssl_stream<beast::tcp_stream> stream,
		const std::shared_ptr<Room> &room,
		const std::string &UUID,
		http::request<Body, http::basic_fields<Allocator>> req)
	{
		std::make_shared<SSLWebsocketSession>(
			room,
			UUID,
			std::move(stream))
			->run(std::move(req));
	}
}

#endif
