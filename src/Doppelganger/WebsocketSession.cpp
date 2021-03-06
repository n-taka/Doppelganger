#ifndef WEBSOCKETSESSION_CPP
#define WEBSOCKETSESSION_CPP

// see
// https://www.boost.org/doc/libs/develop/libs/beast/example/advanced/server-flex/advanced_server_flex.cpp

#include "Doppelganger/WebsocketSession.h"

#include <memory>
#include <string>
#include <sstream>

#include "Doppelganger/Room.h"
#include "Doppelganger/Plugin.h"
#include "Doppelganger/Util/log.h"

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
		const std::shared_ptr<Room> room = room_.lock();

		if (room)
		{
			if (ec)
			{
				return fail(ec, "accept (websocket)");
			}

			room->joinWS(derived().shared_from_this());
			nlohmann::json broadcast = nlohmann::json(nullptr);
			nlohmann::json response = nlohmann::json::object();
			response["sessionUUID"] = UUID_;
			room->broadcastWS("initializeSession", UUID_, broadcast, response);

			// Read a message
			doRead();
		}
	}

	template <class Derived>
	void WebsocketSession<Derived>::doRead()
	{
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
		const std::shared_ptr<Room> room = room_.lock();

		if (room)
		{
			// API
			std::lock_guard<std::mutex> lock(room->mutexRoom_);

			boost::ignore_unused(bytes_transferred);

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
				const std::string APIName = parameters.at("API").get<std::string>();
				const std::string sourceUUID = parameters.at("sessionUUID").get<std::string>();

				nlohmann::json response, broadcast;
				room->plugin_.at(APIName).pluginProcess(
					room,
					parameters.at("parameters"),
					response,
					broadcast);

				// broadcast
				if (!broadcast.is_null() || !response.is_null())
				{
					room->broadcastWS(APIName, sourceUUID, broadcast, response);
				}
			}
			catch (...)
			{
				std::stringstream logContent;
				logContent << "Invalid WS API Call...";
				Util::log(logContent.str(), "ERROR", room->config);
			}

			buffer_.consume(buffer_.size());

			doRead();
		}
	}

	template <class Derived>
	void WebsocketSession<Derived>::doWrite()
	{
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

		queue_.erase(queue_.begin());

		if (!queue_.empty())
		{
			doWrite();
		}
	}

	template <class Derived>
	void WebsocketSession<Derived>::fail(boost::system::error_code ec, char const *what)
	{
		const std::shared_ptr<Room> room = room_.lock();

		if (room)
		{
			{
				std::stringstream ss;
				ss << "WS session \"" << UUID_ << "\" is closed.";
				Util::log(ss.str(), "SYSTEM", room->config);
			}
			// remove mouse cursor
			//   - update parameter on this server
			//   - broadcast message for remove
			room->leaveWS(UUID_);

			// Don't report these
			if (ec == boost::asio::error::operation_aborted ||
				ec == boost::beast::websocket::error::closed)
			{
				return;
			}

			std::stringstream ss;
			ss << what << ": " << ec.message();
			Util::log(ss.str(), "ERROR", room->config);
		}
	}

	template <class Derived>
	WebsocketSession<Derived>::WebsocketSession(
		const std::weak_ptr<Room> &room,
		const std::string &UUID)
		: room_(room), UUID_(UUID)
	{
		std::stringstream ss;
		ss << "New WS session \"" << UUID_ << "\" is created.";
		Util::log(ss.str(), "SYSTEM", room_.lock()->config);
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
}

template Doppelganger::PlainWebsocketSession &Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::derived();
template void Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::onAccept(beast::error_code);
template void Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::doRead();
template void Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::onRead(beast::error_code, std::size_t);
template void Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::doWrite();
template void Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::onWrite(beast::error_code, std::size_t);
template void Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::fail(boost::system::error_code, char const *);
template Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::WebsocketSession(const std::weak_ptr<Room> &, const std::string &);
template void Doppelganger::WebsocketSession<Doppelganger::PlainWebsocketSession>::send(const std::shared_ptr<const std::string> &);

template Doppelganger::SSLWebsocketSession &Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::derived();
template void Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::onAccept(beast::error_code);
template void Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::doRead();
template void Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::onRead(beast::error_code, std::size_t);
template void Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::doWrite();
template void Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::onWrite(beast::error_code, std::size_t);
template void Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::fail(boost::system::error_code, char const *);
template Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::WebsocketSession(const std::weak_ptr<Room> &, const std::string &);
template void Doppelganger::WebsocketSession<Doppelganger::SSLWebsocketSession>::send(const std::shared_ptr<const std::string> &);

#endif
