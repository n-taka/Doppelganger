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

	template <class Derived>
	class HTTPSession
	{
	public:
		HTTPSession(
			const std::weak_ptr<Core> &core,
			beast::flat_buffer buffer);

		void doRead();
		void onRead(beast::error_code ec, std::size_t bytes_transferred);
		void onWrite(bool close, beast::error_code ec, std::size_t bytes_transferred);

	protected:
		beast::flat_buffer buffer_;
		void fail(boost::system::error_code ec, char const *what);

	private:
		Derived &derived();

		class queue
		{
		private:
			enum
			{
				// Maximum number of responses we will queue
				limit = 64
			};

			struct work
			{
				virtual ~work() = default;
				virtual void operator()() = 0;
			};

			HTTPSession &self_;
			std::vector<std::unique_ptr<work>> items_;

		public:
			explicit queue(HTTPSession &self);

			bool isFull() const;
			bool onWrite();
			template <bool isRequest, class Body, class Fields>
			void operator()(http::message<isRequest, Body, Fields> &&msg)
			{
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

				items_.push_back(
					boost::make_unique<workImpl>(self_, std::move(msg)));

				if (items_.size() == 1)
				{
					(*items_.front())();
				}
			}
		};

		queue queue_;
		boost::optional<http::request_parser<http::string_body>> parser_;
		const std::weak_ptr<Core> core_;
	};

	//------------------------------------------------------------------------------

	class PlainHTTPSession
		: public HTTPSession<PlainHTTPSession>,
		  public std::enable_shared_from_this<PlainHTTPSession>
	{
	public:
		PlainHTTPSession(
			const std::weak_ptr<Core> &core,
			beast::tcp_stream &&stream,
			beast::flat_buffer &&buffer);

		void run();
		beast::tcp_stream &stream();
		beast::tcp_stream release_stream();
		void doEof();

	private:
		beast::tcp_stream stream_;
	};

	//------------------------------------------------------------------------------

	class SSLHTTPSession
		: public HTTPSession<SSLHTTPSession>,
		  public std::enable_shared_from_this<SSLHTTPSession>
	{
	public:
		SSLHTTPSession(
			const std::weak_ptr<Core> &core,
			beast::tcp_stream &&stream,
			ssl::context &ctx,
			beast::flat_buffer &&buffer);

		void run();
		beast::ssl_stream<beast::tcp_stream> &stream();
		beast::ssl_stream<beast::tcp_stream> release_stream();
		void doEof();

	private:
		beast::ssl_stream<beast::tcp_stream> stream_;

	private:
		void onHandshake(
			beast::error_code ec,
			std::size_t bytes_used);

		void onShutdown(beast::error_code ec);
	};
}

#endif