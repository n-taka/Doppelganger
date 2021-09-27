#ifndef WEBSOCKETSESSION_CPP
#define WEBSOCKETSESSION_CPP

#include "Doppelganger/WebsocketSession.h"

#include "Doppelganger/Core.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/Logger.h"
#include "Doppelganger/Plugin.h"

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
		// remove mouse cursor
		//   - update parameter on this server
		//   - broadcast message for remove
		{
			room->interfaceParams.cursors.erase(UUID);
			// parameters = {
			//  "sessionUUID": sessionUUID string,
			//  "cursor": {
			//   "remove": boolean value for removing cursor for no longer connected session
			//  }
			// }
			nlohmann::json response, broadcast;
			response = nlohmann::json::object();
			broadcast = nlohmann::json::object();
			broadcast["sessionUUID"] = UUID;
			broadcast["cursor"] = nlohmann::json::object();
			broadcast["cursor"]["remove"] = true;
			room->broadcastWS("syncCursor", UUID, broadcast, response);
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
		nlohmann::json broadcast = nlohmann::json::object();
		nlohmann::json response = nlohmann::json::object();
		response["sessionUUID"] = UUID;
		room->broadcastWS("initializeSession", UUID, broadcast, response);

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

		try
		{
			const std::string payload = boost::beast::buffers_to_string(buffer.data());
			const nlohmann::json parameters = nlohmann::json::parse(payload);
			const std::string &APIName = parameters.at("API").get<std::string>();
			const std::string &sourceUUID = parameters.at("sessionUUID").get<std::string>();
			const Doppelganger::Plugin::API_t &APIFunc = room->core->plugin.at(APIName)->func;

			// WS APIs are called so many times (e.g. syncCursor).
			// So, I simply comment out this logging
			// {
			// 	std::stringstream logContent;
			// 	logContent << "WS";
			// 	logContent << " ";
			// 	logContent << APIName;
			// 	logContent << " ";
			// 	logContent << "(";
			// 	// In some cases, sessionUUID could be NULL (we need to handle the order of initialization...)
			// 	// logContent << parameters.at("sessionUUID").get<std::string>();
			// 	logContent << parameters.at("sessionUUID");
			// 	logContent << ")";
			// 	room->logger.log(logContent.str(), "WSCALL");
			// }

			nlohmann::json response, broadcast;
			APIFunc(room, parameters.at("parameters"), response, broadcast);

			room->broadcastWS(APIName, sourceUUID, broadcast, response);
		}
		catch (...)
		{
			std::stringstream logContent;
			logContent << "Invalid WS API Call...";
			room->logger.log(logContent.str(), "ERROR");
		}

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
