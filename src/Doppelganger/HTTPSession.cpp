#ifndef HTTPSESSION_CPP
#define HTTPSESSION_CPP

#include <memory>
#include <string>
#include <vector>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
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
#include "Doppelganger/HTTPSession.h"
#include "Doppelganger/WebsocketSession.h"
#include "Doppelganger/Plugin.h"
#include "Doppelganger/Util/uuid.h"

namespace
{
	namespace beast = boost::beast;	  // from <boost/beast.hpp>
	namespace http = beast::http;	  // from <boost/beast/http.hpp>
	namespace net = boost::asio;	  // from <boost/asio.hpp>
	using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

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
			return reqPathVec.at(1);
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
						fs::path completePath(room->plugin.at("assets")->pluginDir);
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

						fs::path completePath(core->DoppelgangerRootDir);
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
							room->plugin.at(APIName)->pluginProcess(room, parameters.at("parameters"), response, broadcast);

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
					std::string location = core->completeURL;
					location += "/";
					location += room->UUID_;
					location += "/html/index.html";
					return send(movedPermanently(std::move(req), location));
				}
			}
			else
			{
				// return 301 (moved permanently)
				std::string location = core->completeURL;
				location += "/";
				location += room->UUID_;
				location += "/html/index.html";
				return send(movedPermanently(std::move(req), location));
			}
		}
		else
		{
			// return 301 (moved permanently)
			std::string location = core->completeURL;
			location += "/";
			location += room->UUID_;
			location += "/html/index.html";
			return send(movedPermanently(std::move(req), location));
		}
	}
}

namespace Doppelganger
{
	namespace beast = boost::beast;	  // from <boost/beast.hpp>
	namespace http = beast::http;	  // from <boost/beast/http.hpp>
	namespace net = boost::asio;	  // from <boost/asio.hpp>
	using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

	////
	// HTTPSession
	////
	template <class Derived>
	Derived &HTTPSession<Derived>::derived()
	{
		return static_cast<Derived &>(*this);
	}

	template <class Derived>
	void HTTPSession<Derived>::fail(boost::system::error_code ec, char const *what)
	{
		if (ec == net::ssl::error::stream_truncated)
		{
			return;
		}

		std::stringstream s;
		s << what << ": " << ec.message();
		core_->logger.log(s.str(), "ERROR");
	}

	template <class Derived>
	HTTPSession<Derived>::HTTPSession(
		const std::shared_ptr<Core> &core,
		beast::flat_buffer buffer)
		: queue_(*this), core_(core), buffer_(std::move(buffer))
	{
	}

	template <class Derived>
	void HTTPSession<Derived>::doRead()
	{
		// Construct a new parser for each message
		parser_.emplace();

		// Apply a reasonable limit to the allowed size
		// of the body in bytes to prevent abuse.
		parser_->body_limit(boost::none);

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

	template <class Derived>
	void HTTPSession<Derived>::onRead(beast::error_code ec, std::size_t bytes_transferred)
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

		{
			std::stringstream s;
			s << "Request received: \"" << parser_->get().target().to_string() << "\"";
			core_->logger.log(s.str(), "SYSTEM");
		}

		std::string roomUUID = parseRoomUUID(parser_->get());
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
				if (roomUUID.substr(0, 5) != "room-")
				{
					roomUUID = "room-" + roomUUID;
				}
			}
			const std::shared_ptr<Room> room = std::make_shared<Room>(core_, roomUUID);
			room->setup();
			core_->rooms[roomUUID] = room;
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

	template <class Derived>
	void HTTPSession<Derived>::onWrite(bool close, beast::error_code ec, std::size_t bytes_transferred)
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

	////
	// HTTPSession::queue
	////
	template <class Derived>
	HTTPSession<Derived>::queue::queue(HTTPSession<Derived> &self)
		: self_(self)
	{
		static_assert(limit > 0, "queue limit must be positive");
		items_.reserve(limit);
	}

	template <class Derived>
	bool HTTPSession<Derived>::queue::isFull() const
	{
		return (items_.size() >= limit);
	}

	template <class Derived>
	bool HTTPSession<Derived>::queue::onWrite()
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

	template PlainHTTPSession &HTTPSession<PlainHTTPSession>::derived();
	template void HTTPSession<PlainHTTPSession>::fail(boost::system::error_code, char const *);
	template HTTPSession<PlainHTTPSession>::HTTPSession(const std::shared_ptr<Core> &, beast::flat_buffer);
	template void HTTPSession<PlainHTTPSession>::doRead();
	template void HTTPSession<PlainHTTPSession>::onRead(boost::beast::error_code, std::size_t);
	template void HTTPSession<PlainHTTPSession>::onWrite(bool, boost::beast::error_code, std::size_t);
	template HTTPSession<PlainHTTPSession>::queue::queue(HTTPSession<PlainHTTPSession> &self);
	template bool HTTPSession<PlainHTTPSession>::queue::isFull() const;
	template bool HTTPSession<PlainHTTPSession>::queue::onWrite();

	template Doppelganger::SSLHTTPSession &Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession>::derived();
	template void Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession>::fail(boost::system::error_code, char const *);
	template HTTPSession<SSLHTTPSession>::HTTPSession(const std::shared_ptr<Core> &, beast::flat_buffer);
	template void Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession>::doRead();
	template void Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession>::onRead(beast::error_code, std::size_t);
	template void Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession>::onWrite(bool, beast::error_code, std::size_t);
	template Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession>::queue::queue(Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession> &self);
	template bool Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession>::queue::isFull() const;
	template bool Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession>::queue::onWrite();
};

#endif