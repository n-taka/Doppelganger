#ifndef SSLHTTPSESSION_CPP
#define SSLHTTPSESSION_CPP

#include <memory>

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/bind_executor.hpp>

#include "Doppelganger/HTTPSession.h"

namespace Doppelganger
{
	namespace beast = boost::beast;			// from <boost/beast.hpp>
	namespace ssl = boost::asio::ssl;		// from <boost/asio/ssl.hpp>

	////
	// SSLHTTPSession
	////
	SSLHTTPSession::SSLHTTPSession(
		const std::shared_ptr<Core> &core,
		beast::tcp_stream &&stream,
		ssl::context &ctx,
		beast::flat_buffer &&buffer)
		: HTTPSession<SSLHTTPSession>(
			  core,
			  std::move(buffer)),
		  stream_(std::move(stream), ctx)
	{
	}

	void SSLHTTPSession::run()
	{
		// Set the timeout.
		beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

		// Perform the SSL handshake
		// Note, this is the buffered version of the handshake.
		stream_.async_handshake(
			ssl::stream_base::server,
			buffer_.data(),
			beast::bind_front_handler(
				&SSLHTTPSession::onHandshake,
				shared_from_this()));
	}

	beast::ssl_stream<beast::tcp_stream> &SSLHTTPSession::stream()
	{
		return stream_;
	}

	beast::ssl_stream<beast::tcp_stream> SSLHTTPSession::release_stream()
	{
		return std::move(stream_);
	}

	void SSLHTTPSession::doEof()
	{
		// Set the timeout.
		beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

		// Perform the SSL shutdown
		stream_.async_shutdown(
			beast::bind_front_handler(
				&SSLHTTPSession::onShutdown,
				shared_from_this()));
	}

	void SSLHTTPSession::onHandshake(
		beast::error_code ec,
		std::size_t bytes_used)
	{
		if (ec)
		{
			return fail(ec, "handshake (HTTP)");
		}

		// Consume the portion of the buffer used by the handshake
		buffer_.consume(bytes_used);

		doRead();
	}

	void SSLHTTPSession::onShutdown(beast::error_code ec)
	{
		if (ec)
		{
			return fail(ec, "shutdown (HTTP)");
		}

		// At this point the connection is closed gracefully
	}
};

#endif