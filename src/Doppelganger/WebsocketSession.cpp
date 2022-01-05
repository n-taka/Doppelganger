#ifndef WEBSOCKETSESSION_CPP
#define WEBSOCKETSESSION_CPP

// see
// https://www.boost.org/doc/libs/develop/libs/beast/example/advanced/server-flex/advanced_server_flex.cpp

#include <memory>
#include <string>
#include <sstream>
#include <vector>

#include "Doppelganger/WebsocketSession.h"

#include "Doppelganger/Core.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/Logger.h"
#include "Doppelganger/Plugin.h"

////
// #include "example/common/server_certificate.hpp"

namespace Doppelganger
{
	////
	// WebsocketSession
	////
	template <class Derived>
	Derived &WebsocketSession<Derived>::derived()
	{
		return static_cast<Derived &>(*this);
	}

	template <class Derived>
	void WebsocketSession<Derived>::onAccept(beast::error_code ec)
	{
		if (ec)
		{
			return fail(ec, "accept (websocket)");
		}

		// Add this session to the list of active sessions
		room_->joinWS(derived().shared_from_this());
		// initialize session
		nlohmann::json broadcast = nlohmann::json::object();
		nlohmann::json response = nlohmann::json::object();
		response["sessionUUID"] = UUID_;
		room_->broadcastWS("initializeSession", UUID_, broadcast, response);

		// Read a message
		doRead();
	}

	template <class Derived>
	void WebsocketSession<Derived>::doRead()
	{
		// Read a message into our buffer
		derived().ws().async_read(
			buffer_,
			beast::bind_front_handler(
				&WebsocketSession::onRead,
				derived().shared_from_this()));
	}

	template <class Derived>
	void WebsocketSession<Derived>::onRead(
		beast::error_code ec,
		std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		// This indicates that the websocket_session was closed
		if (ec == websocket::error::closed)
		{
			return;
		}

		if (ec)
		{
			return fail(ec, "read (websocket)");
		}

		try
		{
			const std::string payload = boost::beast::buffers_to_string(buffer_.data());
			const nlohmann::json parameters = nlohmann::json::parse(payload);
			const std::string &APIName = parameters.at("API").get<std::string>();
			const std::string &sourceUUID = parameters.at("sessionUUID").get<std::string>();
			const Doppelganger::Plugin::API_t &APIFunc = room_->core->plugin.at(APIName)->func;

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
			APIFunc(room_, parameters.at("parameters"), response, broadcast);

			room_->broadcastWS(APIName, sourceUUID, broadcast, response);
		}
		catch (...)
		{
			std::stringstream logContent;
			logContent << "Invalid WS API Call...";
			room_->logger.log(logContent.str(), "ERROR");
		}

		// Clear the buffer
		buffer_.consume(buffer_.size());

		// Read another message
		doRead();
	}

	template <class Derived>
	void WebsocketSession<Derived>::doWrite()
	{
		// Read a message into our buffer
		derived().ws().async_write(
			boost::asio::buffer(*queue_.front()),
			beast::bind_front_handler(
				&WebsocketSession::onWrite,
				derived().shared_from_this()));
	}

	template <class Derived>
	void WebsocketSession<Derived>::onWrite(
		beast::error_code ec,
		std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		if (ec)
		{
			return fail(ec, "write (websocket)");
		}

		// Remove the string from the queue
		queue_.erase(queue_.begin());

		// Send the next message if any
		if (!queue_.empty())
		{
			doWrite();
		}
	}

	template <class Derived>
	void WebsocketSession<Derived>::fail(boost::system::error_code ec, char const *what)
	{
		// Remove this session from the list of active sessions
		// If something wrong happens and WS is invalidated, such WS will be removed
		//   in the next write operation.
		{
			std::stringstream ss;
			ss << "WS session \"" << UUID_ << "\" is closed.";
			room_->logger.log(ss.str(), "SYSTEM");
		}
		// remove mouse cursor
		//   - update parameter on this server
		//   - broadcast message for remove
		{
			room_->interfaceParams.cursors.erase(UUID_);
			// parameters = {
			//  "sessionUUID": sessionUUID string,
			//  "cursor": {
			//   "remove": boolean value for removing cursor for no longer connected session
			//  }
			// }
			nlohmann::json response, broadcast;
			response = nlohmann::json::object();
			broadcast = nlohmann::json::object();
			broadcast["sessionUUID"] = UUID_;
			broadcast["cursor"] = nlohmann::json::object();
			broadcast["cursor"]["remove"] = true;
			room_->broadcastWS("syncCursor", UUID_, broadcast, response);
		}
		room_->leaveWS(UUID_);

		// Don't report these
		if (ec == boost::asio::error::operation_aborted ||
			ec == boost::beast::websocket::error::closed)
		{
			return;
		}

		std::stringstream s;
		s << what << ": " << ec.message();
		room_->logger.log(s.str(), "ERROR");
	}

	template <class Derived>
	WebsocketSession<Derived>::WebsocketSession(
		const std::shared_ptr<Room> &room,
		const std::string &UUID)
		: room_(room), UUID_(UUID)
	{
		std::stringstream ss;
		ss << "New WS session \"" << UUID_ << "\" is created.";
		room_->logger.log(ss.str(), "SYSTEM");
	}

	template <class Derived>
	void WebsocketSession<Derived>::send(const std::shared_ptr<const std::string> &ss)
	{
		// Always add to queue
		queue_.push_back(ss);

		// Are we already writing?
		if (queue_.size() > 1)
		{
			return;
		}

		doWrite();
	}

	////
	// PlainWebsocketSession
	////
	PlainWebsocketSession::PlainWebsocketSession(
		const std::shared_ptr<Room> &room,
		const std::string &UUID,
		beast::tcp_stream &&stream)
		: WebsocketSession<PlainWebsocketSession>(room, UUID), ws_(std::move(stream))
	{
	}

	websocket::stream<beast::tcp_stream> &PlainWebsocketSession::ws()
	{
		return ws_;
	}

	////
	// SSLWebsocketSession
	////
	SSLWebsocketSession::SSLWebsocketSession(
		const std::shared_ptr<Room> &room,
		const std::string &UUID,
		beast::ssl_stream<beast::tcp_stream> &&stream)
		: WebsocketSession<SSLWebsocketSession>(room, UUID), ws_(std::move(stream))
	{
	}

	websocket::stream<beast::ssl_stream<beast::tcp_stream>> &SSLWebsocketSession::ws()
	{
		return ws_;
	}
}

template Doppelganger::PlainWebsocketSession &Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::derived();
template void Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::onAccept(beast::error_code);
template void Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::doRead();
template void Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::onRead(beast::error_code, std::size_t);
template void Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::doWrite();
template void Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::onWrite(beast::error_code, std::size_t);
template void Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::fail(boost::system::error_code, char const *);
template Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::WebsocketSession(const std::shared_ptr<Room> &, const std::string &);
template void Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::send(const std::shared_ptr<const std::string> &);

template Doppelganger::SSLWebsocketSession &Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::derived();
template void Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::onAccept(beast::error_code);
template void Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::doRead();
template void Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::onRead(beast::error_code, std::size_t);
template void Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::doWrite();
template void Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::onWrite(beast::error_code, std::size_t);
template void Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::fail(boost::system::error_code, char const *);
template Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::WebsocketSession(const std::shared_ptr<Room> &, const std::string &);
template void Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::send(const std::shared_ptr<const std::string> &);

#endif
