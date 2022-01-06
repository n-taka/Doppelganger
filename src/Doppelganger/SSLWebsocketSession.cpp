#ifndef SSLWEBSOCKETSESSION_CPP
#define SSLWEBSOCKETSESSION_CPP

// see
// https://www.boost.org/doc/libs/develop/libs/beast/example/advanced/server-flex/advanced_server_flex.cpp

#include <memory>
#include <string>

#include "Doppelganger/WebsocketSession.h"

namespace Doppelganger
{
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

#endif
