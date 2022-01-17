#ifndef HTTPSESSION_H
#define HTTPSESSION_H

// see
// https://www.boost.org/doc/libs/develop/libs/beast/example/advanced/server-flex/advanced_server_flex.cpp

#include <memory>
#include <vector>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/make_unique.hpp>
#include <boost/optional.hpp>

namespace Doppelganger
{
	class Core;

	namespace beast = boost::beast;	  // from <boost/beast.hpp>
	namespace http = beast::http;	  // from <boost/beast/http.hpp>
	namespace net = boost::asio;	  // from <boost/asio.hpp>
	namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>
	using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

	// Handles an HTTP server connection.
	// This uses the Curiously Recurring Template Pattern so that
	// the same code works with both SSL streams and regular sockets.

	template <class Derived>
	class HTTPSession
	{
	private:
		// Access the derived class, this is part of
		// the Curiously Recurring Template Pattern idiom.
		Derived &derived();

		// This queue is used for HTTP pipelining.
		class queue
		{
		private:
			enum
			{
				// Maximum number of responses we will queue
				limit = 64
			};

			// The type-erased, saved work item
			struct work
			{
				virtual ~work() = default;
				virtual void operator()() = 0;
			};

			HTTPSession &self_;
			std::vector<std::unique_ptr<work>> items_;

		public:
			explicit queue(HTTPSession &self);

			// Returns `true` if we have reached the queue limit
			bool isFull() const;

			// Called when a message finishes sending
			// Returns `true` if the caller should initiate a read
			bool onWrite();

			// Called by the HTTP handler to send a response.
			template <bool isRequest, class Body, class Fields>
			void operator()(http::message<isRequest, Body, Fields> &&msg)
			{
				// This holds a work item
				struct workImpl : work
				{
					HTTPSession &self_;
					http::message<isRequest, Body, Fields> msg_;

					workImpl(
						HTTPSession &self,
						http::message<isRequest, Body, Fields> &&msg)
						: self_(self), msg_(std::move(msg))
					{
					}

					void operator()()
					{
						http::async_write(
							self_.derived().stream(),
							msg_,
							beast::bind_front_handler(
								&HTTPSession::onWrite,
								self_.derived().shared_from_this(),
								msg_.need_eof()));
					}
				};

				// Allocate and store the work
				items_.push_back(
					boost::make_unique<workImpl>(self_, std::move(msg)));

				// If there was no previous work, start this one
				if (items_.size() == 1)
				{
					(*items_.front())();
				}
			}
		};

		queue queue_;

		// The parser is stored in an optional container so we can
		// construct it from scratch it at the beginning of each new message.
		boost::optional<http::request_parser<http::string_body>> parser_;

		const std::shared_ptr<Core> core_;

	protected:
		beast::flat_buffer buffer_;

		void fail(boost::system::error_code ec, char const *what);

	public:
		// Construct the session
		HTTPSession(
			const std::shared_ptr<Core> &core,
			beast::flat_buffer buffer);

		void doRead();
		void onRead(beast::error_code ec, std::size_t bytes_transferred);
		void onWrite(bool close, beast::error_code ec, std::size_t bytes_transferred);
	};

	//------------------------------------------------------------------------------

	// Handles a plain HTTP connection
	class PlainHTTPSession
		: public HTTPSession<PlainHTTPSession>,
		  public std::enable_shared_from_this<PlainHTTPSession>
	{
		beast::tcp_stream stream_;

	public:
		// Create the session
		PlainHTTPSession(
			const std::shared_ptr<Core> &core,
			beast::tcp_stream &&stream,
			beast::flat_buffer &&buffer);

		// Start the session
		void run();

		// Called by the base class
		beast::tcp_stream &stream();

		// Called by the base class
		beast::tcp_stream release_stream();

		// Called by the base class
		void doEof();
	};

	//------------------------------------------------------------------------------

	// Handles an SSL HTTP connection
	class SSLHTTPSession
		: public HTTPSession<SSLHTTPSession>,
		  public std::enable_shared_from_this<SSLHTTPSession>
	{
		beast::ssl_stream<beast::tcp_stream> stream_;

	public:
		// Create the http_session
		SSLHTTPSession(
			const std::shared_ptr<Core> &core,
			beast::tcp_stream &&stream,
			ssl::context &ctx,
			beast::flat_buffer &&buffer);

		// Start the session
		void run();

		// Called by the base class
		beast::ssl_stream<beast::tcp_stream> &stream();

		// Called by the base class
		beast::ssl_stream<beast::tcp_stream> release_stream();

		// Called by the base class
		void doEof();

	private:
		void onHandshake(
			beast::error_code ec,
			std::size_t bytes_used);

		void onShutdown(beast::error_code ec);
	};
}

#endif