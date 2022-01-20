#ifndef WEBSOCKETSESSION_H
#define WEBSOCKETSESSION_H

// see
// https://www.boost.org/doc/libs/develop/libs/beast/example/advanced/server-flex/advanced_server_flex.cpp

#include <memory>
#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>

namespace Doppelganger
{
	class Room;

	namespace beast = boost::beast;			// from <boost/beast.hpp>
	namespace http = beast::http;			// from <boost/beast/http.hpp>
	namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
	namespace net = boost::asio;			// from <boost/asio.hpp>
	namespace ssl = boost::asio::ssl;		// from <boost/asio/ssl.hpp>
	using tcp = boost::asio::ip::tcp;		// from <boost/asio/ip/tcp.hpp>

	template <class Derived>
	class WebsocketSession
	{
	private:
		Derived &derived();

		template <class Body, class Allocator>
		void doAccept(http::request<Body, http::basic_fields<Allocator>> req)
		{
			derived().ws().set_option(
				websocket::stream_base::timeout::suggested(
					beast::role_type::server));

			derived().ws().set_option(
				websocket::stream_base::decorator(
					[](websocket::response_type &res)
					{
						res.set(http::field::server,
								std::string(BOOST_BEAST_VERSION_STRING) +
									" Doppelganger");
					}));

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

	private:
		beast::flat_buffer buffer_;
		const std::weak_ptr<Room> room_;
		std::vector<std::shared_ptr<const std::string>> queue_;

	public:
		WebsocketSession(
			const std::weak_ptr<Room> &room,
			const std::string &UUID);

		template <class Body, class Allocator>
		void run(http::request<Body, http::basic_fields<Allocator>> req)
		{
			doAccept(std::move(req));
		}

		void send(const std::shared_ptr<const std::string> &ss);

	public:
		const std::string UUID_;
	};

	//------------------------------------------------------------------------------

	class PlainWebsocketSession
		: public WebsocketSession<PlainWebsocketSession>,
		  public std::enable_shared_from_this<PlainWebsocketSession>
	{
	public:
		explicit PlainWebsocketSession(
			const std::weak_ptr<Room> &room,
			const std::string &UUID,
			beast::tcp_stream &&stream);

		websocket::stream<beast::tcp_stream> &ws();

	private:
		websocket::stream<beast::tcp_stream> ws_;
	};

	//------------------------------------------------------------------------------

	class SSLWebsocketSession
		: public WebsocketSession<SSLWebsocketSession>,
		  public std::enable_shared_from_this<SSLWebsocketSession>
	{
	public:
		explicit SSLWebsocketSession(
			const std::weak_ptr<Room> &room,
			const std::string &UUID,
			beast::ssl_stream<beast::tcp_stream> &&stream);

		websocket::stream<beast::ssl_stream<beast::tcp_stream>> &ws();

	private:
		websocket::stream<
			beast::ssl_stream<beast::tcp_stream>>
			ws_;
	};

	//------------------------------------------------------------------------------

	template <class Body, class Allocator>
	void makeWebsocketSession(
		beast::tcp_stream stream,
		const std::weak_ptr<Room> &room,
		const std::string &UUID,
		http::request<Body, http::basic_fields<Allocator>> req)
	{
		std::make_shared<PlainWebsocketSession>(
			room,
			UUID,
			std::move(stream))
			->run(std::move(req));
	}

	template <class Body, class Allocator>
	void makeWebsocketSession(
		beast::ssl_stream<beast::tcp_stream> stream,
		const std::weak_ptr<Room> &room,
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
