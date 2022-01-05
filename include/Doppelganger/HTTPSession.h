#ifndef HTTPSESSION_H
#define HTTPSESSION_H

// #include "example/common/server_certificate.hpp"

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

#if defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#elif defined(__APPLE__)
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#elif defined(__linux__)
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include "Doppelganger/Core.h"
#include "Doppelganger/Logger.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/WebsocketSession.h"
#include "Doppelganger/Plugin.h"
#include "Doppelganger/Util/uuid.h"

namespace
{
	namespace beast = boost::beast;			// from <boost/beast.hpp>
	namespace http = beast::http;			// from <boost/beast/http.hpp>
	namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
	namespace net = boost::asio;			// from <boost/asio.hpp>
	namespace ssl = boost::asio::ssl;		// from <boost/asio/ssl.hpp>
	using tcp = boost::asio::ip::tcp;		// from <boost/asio/ip/tcp.hpp>

	beast::string_view
	mime_type(const fs::path &p)
	{
		const std::string ext = p.extension().string();
		if (ext == ".htm")
			return "text/html";
		if (ext == ".html")
			return "text/html";
		if (ext == ".php")
			return "text/html";
		if (ext == ".css")
			return "text/css";
		if (ext == ".txt")
			return "text/plain";
		if (ext == ".js")
			return "application/javascript";
		if (ext == ".json")
			return "application/json";
		if (ext == ".xml")
			return "application/xml";
		if (ext == ".swf")
			return "application/x-shockwave-flash";
		if (ext == ".flv")
			return "video/x-flv";
		if (ext == ".png")
			return "image/png";
		if (ext == ".jpe")
			return "image/jpeg";
		if (ext == ".jpeg")
			return "image/jpeg";
		if (ext == ".jpg")
			return "image/jpeg";
		if (ext == ".gif")
			return "image/gif";
		if (ext == ".bmp")
			return "image/bmp";
		if (ext == ".ico")
			return "image/vnd.microsoft.icon";
		if (ext == ".tiff")
			return "image/tiff";
		if (ext == ".tif")
			return "image/tiff";
		if (ext == ".svg")
			return "image/svg+xml";
		if (ext == ".svgz")
			return "image/svg+xml";
		return "application/text";
	}

	template <class Body, class Allocator>
	std::string parseRoomUUID(const http::request<Body, http::basic_fields<Allocator>> &req)
	{
		fs::path reqPath(req.target().to_string());
		reqPath.make_preferred();

		std::vector<std::string> reqPathVec;
		for (const auto &p : reqPath)
		{
			reqPathVec.push_back(p.string());
		}

		if (reqPathVec.size() >= 2)
		{
			if (reqPathVec.at(1).substr(0, 5) == "room-")
			{
				return reqPathVec.at(1).substr(5);
			}
			else
			{
				return reqPathVec.at(1);
			}
		}
		else
		{
			return std::string("");
		}
	}

	template <class Body, class Allocator>
	http::response<http::string_body> badRequest(
		http::request<Body, http::basic_fields<Allocator>> &&req,
		beast::string_view why)
	{
		// Returns a bad request response
		http::response<http::string_body> res{http::status::bad_request, req.version()};
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = why.to_string();
		res.prepare_payload();
		return res;
	}

	template <class Body, class Allocator>
	http::response<http::string_body> notFound(
		http::request<Body, http::basic_fields<Allocator>> &&req,
		beast::string_view target)
	{
		// Returns a not found response
		http::response<http::string_body> res{http::status::not_found, req.version()};
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = "The resource '" + target.to_string() + "' was not found.";
		res.prepare_payload();
		return res;
	}

	template <class Body, class Allocator>
	http::response<http::string_body> serverError(
		http::request<Body, http::basic_fields<Allocator>> &&req,
		beast::string_view what)
	{
		// Returns a server error response
		http::response<http::string_body> res{http::status::internal_server_error, req.version()};
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = "An error occurred: '" + what.to_string() + "'";
		res.prepare_payload();
		return res;
	}

	template <class Body, class Allocator>
	http::response<http::string_body> movedPermanently(
		http::request<Body, http::basic_fields<Allocator>> &&req,
		beast::string_view location)
	{
		// Returns a moved permanently
		http::response<http::string_body> res{http::status::moved_permanently, req.version()};
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.set(http::field::location, location);
		res.keep_alive(req.keep_alive());
		res.prepare_payload();
		return res;
	}

	template <class Body, class Allocator, class Send>
	void handleRequest(const std::shared_ptr<Doppelganger::Core> &core,
					   const std::shared_ptr<Doppelganger::Room> &room,
					   http::request<Body, http::basic_fields<Allocator>> &&req,
					   Send &&send)
	{
		// Make sure we can handle the method
		if (req.method() != http::verb::head &&
			req.method() != http::verb::get &&
			req.method() != http::verb::post)
		{
			return send(badRequest(std::move(req), "Unknown HTTP-method"));
		}

		// Request path must be absolute and not contain "..".
		if (req.target().empty() ||
			req.target()[0] != '/' ||
			req.target().find("..") != beast::string_view::npos)
		{
			return send(badRequest(std::move(req), "Illegal request-target"));
		}

		// resource
		// http://example.com/<roomUUID>/<path>/<to>/<resource>
		// API
		// http://example.com/<roomUUID>/<APIName>
		// API (module.js)
		// http://example.com/<roomUUID>/plugin/APIName_version/module.js
		fs::path reqPath(req.target().to_string());
		reqPath.make_preferred();
		// reqPathVec
		// {"/", "<roomUUID>", "<APIName>", ... }
		std::vector<std::string> reqPathVec;
		for (const auto &p : reqPath)
		{
			reqPathVec.push_back(p.string());
		}

		if (reqPathVec.size() >= 2)
		{
			if (reqPathVec.at(1).substr(0, 5) == "room-")
			{
				if (reqPathVec.size() >= 3)
				{
					if (reqPathVec.at(2) == "css" || reqPathVec.at(2) == "html" || reqPathVec.at(2) == "icon" || reqPathVec.at(2) == "js")
					{
						// resource
						fs::path completePath(room->core->config.at("plugin").at("dir").get<std::string>());
						completePath.make_preferred();
						{
							// find assets plugin
							std::string dirName("assets");
							dirName += "_";
							std::string installedVersion(room->core->plugin.at("assets")->installedVersion);
							if (installedVersion == "latest")
							{
								installedVersion = room->core->plugin.at("assets")->parameters.at("versions").at(0).at("version").get<std::string>();
							}
							dirName += installedVersion;
							completePath.append(dirName);
						}
						for (int pIdx = 2; pIdx < reqPathVec.size(); ++pIdx)
						{
							completePath.append(reqPathVec.at(pIdx));
						}

						// Attempt to open the file
						beast::error_code ec;
						http::file_body::value_type body;
						body.open(completePath.string().c_str(), beast::file_mode::scan, ec);

						// Handle the case where the file doesn't exist
						if (ec == boost::system::errc::no_such_file_or_directory)
						{
							return send(notFound(std::move(req), req.target()));
						}
						// Handle an unknown error
						if (ec)
						{
							return send(serverError(std::move(req), ec.message()));
						}

						// Cache the size since we need it after the move
						const auto size = body.size();

						if (req.method() == http::verb::head)
						{
							// Respond to HEAD request
							http::response<http::empty_body> res{http::status::ok, req.version()};
							res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
							res.set(http::field::content_type, mime_type(completePath));
							res.content_length(size);
							res.keep_alive(req.keep_alive());
							return send(std::move(res));
						}
						else if (req.method() == http::verb::get)
						{
							// Respond to GET request
							http::response<http::file_body> res{
								std::piecewise_construct,
								std::make_tuple(std::move(body)),
								std::make_tuple(http::status::ok, req.version())};
							res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
							res.set(http::field::content_type, mime_type(completePath));
							res.content_length(size);
							res.keep_alive(req.keep_alive());
							return send(std::move(res));
						}
						else
						{
							return send(badRequest(std::move(req), "Illegal request"));
						}
					}
					else if (reqPathVec.at(2) == "plugin")
					{
						// resource
						fs::path completePath(core->systemParams.workingDir);
						completePath.make_preferred();
						for (int pIdx = 2; pIdx < reqPathVec.size(); ++pIdx)
						{
							completePath.append(reqPathVec.at(pIdx));
						}

						// Attempt to open the file
						beast::error_code ec;
						http::file_body::value_type body;
						body.open(completePath.string().c_str(), beast::file_mode::scan, ec);

						// Handle the case where the file doesn't exist
						if (ec == beast::errc::no_such_file_or_directory)
						{
							return send(notFound(std::move(req), req.target()));
						}
						// Handle an unknown error
						if (ec)
						{
							return send(serverError(std::move(req), ec.message()));
						}

						// Cache the size since we need it after the move
						const auto size = body.size();

						// Respond to HEAD request
						if (req.method() == http::verb::head)
						{
							http::response<http::empty_body> res{http::status::ok, req.version()};
							res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
							res.set(http::field::content_type, mime_type(completePath));
							res.content_length(size);
							res.keep_alive(req.keep_alive());
							return send(std::move(res));
						}
						else if (req.method() == http::verb::get)
						{
							// Respond to GET request
							http::response<http::file_body> res{
								std::piecewise_construct,
								std::make_tuple(std::move(body)),
								std::make_tuple(http::status::ok, req.version())};
							res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
							res.set(http::field::content_type, mime_type(completePath));
							res.content_length(size);
							res.keep_alive(req.keep_alive());
							return send(std::move(res));
						}
						else
						{
							return send(badRequest(std::move(req), "Illegal request"));
						}
					}
					else
					{
						// API
						try
						{
							std::string taskUUID;
							{
								std::lock_guard<std::mutex> lock(room->interfaceParams.mutex);
								taskUUID = Doppelganger::Util::uuid("task-");
								room->interfaceParams.taskUUIDInProgress.insert(taskUUID);

								nlohmann::json broadcast = nlohmann::json::object();
								nlohmann::json response = nlohmann::json::object();
								broadcast["isBusy"] = (room->interfaceParams.taskUUIDInProgress.size() > 0);
								room->broadcastWS("isServerBusy", std::string(""), broadcast, response);
							}

							const std::string &APIName = reqPathVec.at(2);
							const Doppelganger::Plugin::API_t &APIFunc = core->plugin.at(APIName)->func;

							nlohmann::json parameters = nlohmann::json::object();
							boost::optional<std::uint64_t> size = req.payload_size();
							if (size && *size > 0)
							{
								parameters = nlohmann::json::parse(req.body());
							}
							{
								std::stringstream logContent;
								logContent << req.method_string();
								logContent << " ";
								logContent << req.target().to_string();
								logContent << " ";
								logContent << "(";
								// In some cases, sessionUUID could be NULL (we need to handle the order of initialization...)
								// logContent << parameters.at("sessionUUID").get<std::string>();
								logContent << parameters.at("sessionUUID");
								logContent << ")";
								room->logger.log(logContent.str(), "APICALL");
							}

							nlohmann::json response, broadcast;
							APIFunc(room, parameters.at("parameters"), response, broadcast);

							{
								std::lock_guard<std::mutex> lock(room->interfaceParams.mutex);
								room->interfaceParams.taskUUIDInProgress.erase(taskUUID);

								nlohmann::json broadcast = nlohmann::json::object();
								nlohmann::json response = nlohmann::json::object();
								broadcast["isBusy"] = (room->interfaceParams.taskUUIDInProgress.size() > 0);
								room->broadcastWS("isServerBusy", std::string(""), broadcast, response);
							}

							// broadcast
							if (!broadcast.empty())
							{
								room->broadcastWS(APIName, std::string(""), broadcast, response);
							}

							// response
							const std::string responseStr = response.dump();
							http::string_body::value_type payloadBody = responseStr;
							http::response<http::string_body> res{
								std::piecewise_construct,
								std::make_tuple(std::move(payloadBody)),
								std::make_tuple(http::status::ok, req.version())};
							res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
							res.set(http::field::content_type, "application/json");
							res.content_length(responseStr.size());
							res.keep_alive(req.keep_alive());

							return send(std::move(res));
						}
						catch (...)
						{
							return send(badRequest(std::move(req), "Invalid API call."));
						}
					}
				}
				else
				{
					// return 301 (moved permanently)
					std::string location = core->config.at("server").at("completeURL").get<std::string>();
					location += "/";
					location += room->UUID;
					location += "/html/index.html";
					return send(movedPermanently(std::move(req), location));
				}
			}
			else
			{
				// return 301 (moved permanently)
				std::string location = core->config.at("server").at("completeURL").get<std::string>();
				location += "/";
				location += room->UUID;
				location += "/html/index.html";
				return send(movedPermanently(std::move(req), location));
			}
		}
	}
}

namespace Doppelganger
{
	namespace beast = boost::beast;			// from <boost/beast.hpp>
	namespace http = beast::http;			// from <boost/beast/http.hpp>
	namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
	namespace net = boost::asio;			// from <boost/asio.hpp>
	namespace ssl = boost::asio::ssl;		// from <boost/asio/ssl.hpp>
	using tcp = boost::asio::ip::tcp;		// from <boost/asio/ip/tcp.hpp>

	// Handles an HTTP server connection.
	// This uses the Curiously Recurring Template Pattern so that
	// the same code works with both SSL streams and regular sockets.

	template <class Derived>
	class HTTPSession
	{
	private:
		// Access the derived class, this is part of
		// the Curiously Recurring Template Pattern idiom.
		Derived &derived()
		{
			return static_cast<Derived &>(*this);
		}

		// This queue is used for HTTP pipelining.
		class queue
		{
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
			explicit queue(HTTPSession &self)
				: self_(self)
			{
				static_assert(limit > 0, "queue limit must be positive");
				items_.reserve(limit);
			}

			// Returns `true` if we have reached the queue limit
			bool isFull() const
			{
				return items_.size() >= limit;
			}

			// Called when a message finishes sending
			// Returns `true` if the caller should initiate a read
			bool onWrite()
			{
				BOOST_ASSERT(!items_.empty());
				auto const wasFull = isFull();
				items_.erase(items_.begin());
				if (!items_.empty())
				{
					(*items_.front())();
				}
				return wasFull;
			}

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

					void
					operator()()
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

		void fail(boost::system::error_code ec, char const *what)
		{
			if (ec == net::ssl::error::stream_truncated)
			{
				return;
			}

			std::stringstream s;
			s << what << ": " << ec.message();
			core_->logger.log(s.str(), "ERROR");
		}

	public:
		// Construct the session
		HTTPSession(
			const std::shared_ptr<Core> &core,
			beast::flat_buffer buffer)
			: queue_(*this), core_(core), buffer_(std::move(buffer))
		{
		}

		void doRead()
		{
			// Construct a new parser for each message
			parser_.emplace();

			// Apply a reasonable limit to the allowed size
			// of the body in bytes to prevent abuse.
			parser_->body_limit(10000);

			// Set the timeout.
			beast::get_lowest_layer(
				derived().stream())
				.expires_after(std::chrono::seconds(30));

			// Read a request using the parser-oriented interface
			http::async_read(
				derived().stream(),
				buffer_,
				*parser_,
				beast::bind_front_handler(
					&HTTPSession::onRead,
					derived().shared_from_this()));
		}

		void onRead(beast::error_code ec, std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);

			// This means they closed the connection
			if (ec == http::error::end_of_stream)
			{
				return derived().doEof();
			}

			if (ec)
			{
				return fail(ec, "read (HTTP)");
			}

			std::string roomUUID = parseRoomUUID(parser_->get());
			{
				std::stringstream s;
				s << "Request received: \"" << parser_->get().target().to_string() << "\"";
				core_->logger.log(s.str(), "SYSTEM");
			}

			if (roomUUID == "favicon.ico")
			{
				// do nothing
				// TODO: prepare favion.ico
			}
			else if (roomUUID.size() <= 0 || core_->rooms.find(roomUUID) == core_->rooms.end())
			{
				// create new room
				if (roomUUID.size() <= 0)
				{
					roomUUID = Util::uuid("room-");
				}
				else
				{
					// add prefix
					roomUUID = "room-" + roomUUID;
				}
				const std::shared_ptr<Room> room = std::make_shared<Room>(roomUUID, core_);
				core_->rooms[roomUUID] = std::move(room);

				// todo check...
				// // return 301 (moved permanently)
				// std::string location = core_->config.at("server").at("completeURL").get<std::string>();
				// location += "/";
				// location += roomUUID;
				// return queue_(movedPermanently(std::move(parser_->release()), location));
				handleRequest(core_, room, parser_->release(), queue_);
			}
			else
			{
				const std::shared_ptr<Room> &room = core_->rooms.at(roomUUID);
				// See if it is a WebSocket Upgrade
				if (boost::beast::websocket::is_upgrade(parser_->get()))
				{
					// Disable the timeout.
					// The websocket::stream uses its own timeout settings.
					beast::get_lowest_layer(derived().stream()).expires_never();

					// Create a websocket session, transferring ownership
					// of both the socket and the HTTP request.
					const std::string sessionUUID = Util::uuid("session-");
					makeWebsocketSession(derived().release_stream(), room, sessionUUID, parser_->release());
					return;
				}
				else
				{
					// Send the response
					handleRequest(core_, room, parser_->release(), queue_);
					return;
				}
			}

			// If we aren't at the queue limit, try to pipeline another request
			if (!queue_.isFull())
			{
				doRead();
			}
		}

		void onWrite(bool close, beast::error_code ec, std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);

			if (ec)
			{
				return fail(ec, "write (HTTP)");
			}

			if (close)
			{
				// This means we should close the connection, usually because
				// the response indicated the "Connection: close" semantic.
				return derived().doEof();
			}

			// Inform the queue that a write completed
			if (queue_.onWrite())
			{
				// Read another request
				doRead();
			}
		}
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
			beast::flat_buffer &&buffer)
			: HTTPSession<PlainHTTPSession>(
				  core,
				  std::move(buffer)),
			  stream_(std::move(stream))
		{
		}

		// Start the session
		void run()
		{
			this->doRead();
		}

		// Called by the base class
		beast::tcp_stream &stream()
		{
			return stream_;
		}

		// Called by the base class
		beast::tcp_stream release_stream()
		{
			return std::move(stream_);
		}

		// Called by the base class
		void doEof()
		{
			// Send a TCP shutdown
			beast::error_code ec;
			stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

			// At this point the connection is closed gracefully
		}
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
			beast::flat_buffer &&buffer)
			: HTTPSession<SSLHTTPSession>(
				  core,
				  std::move(buffer)),
			  stream_(std::move(stream), ctx)
		{
		}

		// Start the session
		void run()
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

		// Called by the base class
		beast::ssl_stream<beast::tcp_stream> &stream()
		{
			return stream_;
		}

		// Called by the base class
		beast::ssl_stream<beast::tcp_stream> release_stream()
		{
			return std::move(stream_);
		}

		// Called by the base class
		void doEof()
		{
			// Set the timeout.
			beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

			// Perform the SSL shutdown
			stream_.async_shutdown(
				beast::bind_front_handler(
					&SSLHTTPSession::onShutdown,
					shared_from_this()));
		}

	private:
		void onHandshake(
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

		void onShutdown(beast::error_code ec)
		{
			if (ec)
			{
				return fail(ec, "shutdown (HTTP)");
			}

			// At this point the connection is closed gracefully
		}
	};
}

#endif