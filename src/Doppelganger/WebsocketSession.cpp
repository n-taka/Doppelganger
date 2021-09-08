#ifndef WEBSOCKETSESSION_CPP
#define WEBSOCKETSESSION_CPP

#include "Doppelganger/WebsocketSession.h"

#include "Doppelganger/Room.h"
#include "Doppelganger/Logger.h"

namespace Doppelganger
{
	WebsocketSession::WebsocketSession(boost::asio::ip::tcp::socket socket,
									   const std::string &UUID_,
									   const std::shared_ptr<Room> &room_)
		: UUID(UUID_), ws(std::move(socket)), room(room_)
	{
		std::stringstream ss;
		ss << "New WS session \"" << UUID << "\" is created.";
		room->logger.log(ss.str(), "SYSTEM");
	}

	WebsocketSession::~WebsocketSession()
	{
	}

	void WebsocketSession::fail(boost::system::error_code ec, char const *what)
	{
		// Remove this session from the list of active sessions
		// If something wrong happens and WS is invalidated, such WS will be removed 
		//   in the next write operation.
		{
			std::stringstream ss;
			ss << "WS session \"" << UUID << "\" is closed.";
			room->logger.log(ss.str(), "SYSTEM");
		}
		room->leaveWS(UUID);

		// Don't report these
		if (ec == boost::asio::error::operation_aborted ||
			ec == boost::beast::websocket::error::closed)
		{
			return;
		}

		std::stringstream s;
		s << what << ": " << ec.message();
		room->logger.log(s.str(), "ERROR");
	}

	void WebsocketSession::onAccept(boost::system::error_code ec)
	{
		// Handle the error, if any
		if (ec)
		{
			return fail(ec, "accept (websocket)");
		}

		// Add this session to the list of active sessions
		room->joinWS(shared_from_this());

		// initialize session
		nlohmann::json json;
		json["task"] = "initializeSession";
		json["parameters"] = nlohmann::json::object();
		json.at("parameters")["UUID"] = UUID;
		const std::string payload = json.dump();
		const std::shared_ptr<const std::string> ss = std::make_shared<const std::string>(std::move(payload));
		send(ss);

		// start reading a message
		ws.async_read(
			buffer,
			[sp = shared_from_this()](
				boost::system::error_code ec, std::size_t bytes)
			{
				sp->onRead(ec, bytes);
			});
	}

	void WebsocketSession::onRead(boost::system::error_code ec, std::size_t bytes_transferred)
	{
		// Handle the error, if any
		if (ec)
		{
			return fail(ec, "read (websocket)");
		}

		const std::string payload = boost::beast::buffers_to_string(buffer.data());
		const nlohmann::json payloadJson = nlohmann::json::parse(payload);
// ===== todo call API =====
#if 0
		// Send to all connections
		pantry->broadcastWS(payload, std::unordered_set<int>({parameters.at("sessionId")}));

		// Clear the buffer
		buffer.consume(buffer.size());

		// Read another message
		ws.async_read(
			buffer,
			[sp = shared_from_this()](
				boost::system::error_code ec, std::size_t bytes)
			{
				sp->onRead(ec, bytes);
			});
#endif
	}

	void WebsocketSession::send(const std::shared_ptr<const std::string> &ss)
	{
		// Always add to queue
		queue.push_back(ss);

		// Are we already writing?
		if (queue.size() > 1)
		{
			return;
		}

		// We are not currently writing, so send this immediately
		ws.async_write(
			boost::asio::buffer(*queue.front()),
			[sp = shared_from_this()](
				boost::system::error_code ec, std::size_t bytes)
			{
				sp->onWrite(ec, bytes);
			});
	}

	void WebsocketSession::onWrite(boost::system::error_code ec, std::size_t)
	{
		// Handle the error, if any
		if (ec)
		{
			return fail(ec, "write (websocket)");
		}

		// Remove the string from the queue
		queue.erase(queue.begin());

		// Send the next message if any
		if (!queue.empty())
			ws.async_write(
				boost::asio::buffer(*queue.front()),
				[sp = shared_from_this()](
					boost::system::error_code ec, std::size_t bytes)
				{
					sp->onWrite(ec, bytes);
				});
	}
}

#endif
