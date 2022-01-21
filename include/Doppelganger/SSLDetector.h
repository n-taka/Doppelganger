#ifndef SSLDETECTOR_H
#define SSLDETECTOR_H

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/make_unique.hpp>
#include <boost/optional.hpp>

#include "Doppelganger/Core.h"
#include "Doppelganger/HTTPSession.h"
#include "Doppelganger/Util/log.h"

namespace Doppelganger
{
	namespace beast = boost::beast;			// from <boost/beast.hpp>
	namespace http = beast::http;			// from <boost/beast/http.hpp>
	namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
	namespace net = boost::asio;			// from <boost/asio.hpp>
	namespace ssl = boost::asio::ssl;		// from <boost/asio/ssl.hpp>
	using tcp = boost::asio::ip::tcp;		// from <boost/asio/ip/tcp.hpp>

	class SSLDetector : public std::enable_shared_from_this<SSLDetector>
	{
	private:
		const std::weak_ptr<Core> core_;
		beast::tcp_stream stream_;
		ssl::context &ctx_;
		beast::flat_buffer buffer_;

		void fail(boost::system::error_code ec, char const *what)
		{
			if (ec == net::ssl::error::stream_truncated)
			{
				return;
			}

			{
				std::stringstream ss;
				ss << what << ": " << ec.message();
				Util::log(ss.str(), "ERROR", core_.lock()->config_);
			}
		}

	public:
		explicit SSLDetector(
			const std::weak_ptr<Core> &core,
			tcp::socket &&socket,
			ssl::context &ctx)
			: core_(core), stream_(std::move(socket)), ctx_(ctx)
		{
		}

		// Launch the detector
		void run()
		{
			// We need to be executing within a strand to perform async operations
			// on the I/O objects in this session. Although not strictly necessary
			// for single-threaded contexts, this example code is written to be
			// thread-safe by default.
			net::dispatch(
				stream_.get_executor(),
				beast::bind_front_handler(
					&SSLDetector::onRun,
					this->shared_from_this()));
		}

		void onRun()
		{
			// Set the timeout.
			stream_.expires_after(std::chrono::seconds(30));

			beast::async_detect_ssl(
				stream_,
				buffer_,
				beast::bind_front_handler(
					&SSLDetector::onDetect,
					this->shared_from_this()));
		}

		void onDetect(beast::error_code ec, bool result)
		{
			if (ec)
			{
				return fail(ec, "detect (SSL detector)");
			}

			if (result)
			{
				// Launch SSL session
				std::make_shared<SSLHTTPSession>(
					core_,
					std::move(stream_),
					ctx_,
					std::move(buffer_))
					->run();
			}
			else
			{
				// Launch plain session
				std::make_shared<PlainHTTPSession>(
					core_,
					std::move(stream_),
					std::move(buffer_))
					->run();
			}
		}
	};
}

#endif
