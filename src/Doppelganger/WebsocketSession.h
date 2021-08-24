#ifndef WEBSOCKETSESSION_H
#define WEBSOCKETSESSION_H

#include <memory>
#include <string>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace Doppelganger
{
	class Room;

	class WebsocketSession : public std::enable_shared_from_this<WebsocketSession>
	{
		boost::beast::flat_buffer buffer;
		boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws;
		const std::shared_ptr<Room> &room;
		std::vector<std::shared_ptr<const std::string>> queue;
		std::string sessionUUID;

		void fail(boost::system::error_code ec, char const *what);
		void onAccept(boost::system::error_code ec);
		void onRead(boost::system::error_code ec, std::size_t bytes_transferred);
		void onWrite(boost::system::error_code ec, std::size_t bytes_transferred);

	public:
		WebsocketSession(boost::asio::ip::tcp::socket socket,
						 const std::shared_ptr<Room> &room_);

		~WebsocketSession();

		template <class Body, class Allocator>
		void run(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req);

		// Send a message
		void send(const std::shared_ptr<const std::string> &ss);
	};

	template <class Body, class Allocator>
	void WebsocketSession::run(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req)
	{
		// Accept the websocket handshake
		ws.async_accept(
			req,
			std::bind(
				&WebsocketSession::onAccept,
				shared_from_this(),
				std::placeholders::_1));
	}
}

#endif
