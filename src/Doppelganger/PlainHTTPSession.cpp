#ifndef PLAINHTTPSESSION_CPP
#define PLAINHTTPSESSION_CPP

#include <memory>

#include <boost/beast/core.hpp>

#include "Doppelganger/HTTPSession.h"

namespace Doppelganger
{
	namespace beast = boost::beast;	  // from <boost/beast.hpp>
	using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

	class Core;

	////
	// PlainHTTPSession
	////
	PlainHTTPSession::PlainHTTPSession(
		const std::weak_ptr<Core> &core,
		beast::tcp_stream &&stream,
		beast::flat_buffer &&buffer)
		: HTTPSession<PlainHTTPSession>(
			  core,
			  std::move(buffer)),
		  stream_(std::move(stream))
	{
	}

	void PlainHTTPSession::run()
	{
		this->doRead();
	}

	// Called by the base class
	beast::tcp_stream &PlainHTTPSession::stream()
	{
		return stream_;
	}

	// Called by the base class
	beast::tcp_stream PlainHTTPSession::release_stream()
	{
		return std::move(stream_);
	}

	// Called by the base class
	void PlainHTTPSession::doEof()
	{
		// Send a TCP shutdown
		beast::error_code ec;
		stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

		// At this point the connection is closed gracefully
	}
};

#endif