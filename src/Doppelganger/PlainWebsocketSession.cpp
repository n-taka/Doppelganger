#ifndef PLAINWEBSOCKETSESSION_CPP
#define PLAINWEBSOCKETSESSION_CPP

// see
// https://www.boost.org/doc/libs/develop/libs/beast/example/advanced/server-flex/advanced_server_flex.cpp

#include <memory>
#include <string>

#include "Doppelganger/WebsocketSession.h"

namespace Doppelganger
{
	////
	// PlainWebsocketSession
	////
	PlainWebsocketSession::PlainWebsocketSession(
		const std::weak_ptr<Room> &room,
		const std::string &UUID,
		beast::tcp_stream &&stream)
		: WebsocketSession<PlainWebsocketSession>(room, UUID), ws_(std::move(stream))
	{
	}

	websocket::stream<beast::tcp_stream> &PlainWebsocketSession::ws()
	{
		return ws_;
	}
}

#endif
