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
#include "Doppelganger/Room.h"
#include "Doppelganger/HTTPSession.h"
#include "Doppelganger/WebsocketSession.h"
#include "Doppelganger/Plugin.h"
#include "Doppelganger/Util/uuid.h"
#include "Doppelganger/Util/log.h"

namespace Doppelganger
{
	namespace beast = boost::beast;	  // from <boost/beast.hpp>
	namespace http = beast::http;	  // from <boost/beast/http.hpp>
	namespace net = boost::asio;	  // from <boost/asio.hpp>
	using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

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

		{
			std::stringstream ss;
			ss << what << ": " << ec.message();
			Util::log(ss.str(), "ERROR", core_.lock()->dataDir_, core_.lock()->logConfig_);
		}
	}

	template <class Derived>
	HTTPSession<Derived>::HTTPSession(
		const std::weak_ptr<Core> &core,
		beast::flat_buffer buffer)
		: buffer_(std::move(buffer)), queue_(*this), core_(core)
	{
	}

	template <class Derived>
	void HTTPSession<Derived>::doRead()
	{
		parser_.emplace();
		// disable parser body_limit
		// this is not the best practice, but works
		parser_->body_limit(boost::none);

		// Set the timeout.
		beast::get_lowest_layer(
			derived().stream())
			.expires_after(std::chrono::seconds(30));

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
			std::stringstream ss;
			ss << "Request received: \"" << parser_->get().target().to_string() << "\"";
			Util::log(ss.str(), "SYSTEM", core_.lock()->dataDir_, core_.lock()->logConfig_);
		}

		std::string roomUUID = parseRoomUUID(parser_->get());
		if (roomUUID == "favicon.ico")
		{
			// do nothing
			// TODO: prepare favion.ico
		}
		else if (roomUUID.size() <= 0 || core_.lock()->rooms_.find(roomUUID) == core_.lock()->rooms_.end())
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
			const std::shared_ptr<Room> room = std::make_shared<Room>();
			nlohmann::json configCore;
			core_.lock()->to_json(configCore);
			room->setup(roomUUID, configCore);
			core_.lock()->rooms_[roomUUID] = room;
			handleRequest(core_, room, parser_->release(), queue_);
		}
		else
		{
			const std::shared_ptr<Room> &room = core_.lock()->rooms_.at(roomUUID);
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

		if (queue_.onWrite())
		{
			doRead();
		}
	}

	// HTTPSession::queue
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
	template HTTPSession<PlainHTTPSession>::HTTPSession(const std::weak_ptr<Core> &, beast::flat_buffer);
	template void HTTPSession<PlainHTTPSession>::doRead();
	template void HTTPSession<PlainHTTPSession>::onRead(boost::beast::error_code, std::size_t);
	template void HTTPSession<PlainHTTPSession>::onWrite(bool, boost::beast::error_code, std::size_t);
	template HTTPSession<PlainHTTPSession>::queue::queue(HTTPSession<PlainHTTPSession> &self);
	template bool HTTPSession<PlainHTTPSession>::queue::isFull() const;
	template bool HTTPSession<PlainHTTPSession>::queue::onWrite();

	template Doppelganger::SSLHTTPSession &Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession>::derived();
	template void Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession>::fail(boost::system::error_code, char const *);
	template HTTPSession<SSLHTTPSession>::HTTPSession(const std::weak_ptr<Core> &, beast::flat_buffer);
	template void Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession>::doRead();
	template void Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession>::onRead(beast::error_code, std::size_t);
	template void Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession>::onWrite(bool, beast::error_code, std::size_t);
	template Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession>::queue::queue(Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession> &self);
	template bool Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession>::queue::isFull() const;
	template bool Doppelganger::HTTPSession<Doppelganger::SSLHTTPSession>::queue::onWrite();
};

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
	void openAndSendResource(const fs::path &completePath,
							 http::request<Body, http::basic_fields<Allocator>> &&req,
							 Send &&send)
	{
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

	template <class Body, class Allocator, class Send>
	void handleRequest(const std::weak_ptr<Doppelganger::Core> &core,
					   const std::weak_ptr<Doppelganger::Room> &room,
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
						fs::path completePath(room.lock()->plugin_.at("assets").dir_);
						for (int pIdx = 2; pIdx < reqPathVec.size(); ++pIdx)
						{
							completePath.append(reqPathVec.at(pIdx));
						}

						openAndSendResource(completePath, std::move(req), send);
					}
					else if (reqPathVec.at(2) == "plugin")
					{
						// resource
						fs::path completePath(core.lock()->DoppelgangerRootDir_);
						for (int pIdx = 2; pIdx < reqPathVec.size(); ++pIdx)
						{
							completePath.append(reqPathVec.at(pIdx));
						}

						openAndSendResource(completePath, std::move(req), send);
					}
					else
					{
						// API
						std::lock_guard<std::mutex> lock(room.lock()->mutexRoom_);
						try
						{
							{
								nlohmann::json serverBusyBroadcast = nlohmann::json::object();
								serverBusyBroadcast["isBusy"] = true;
								room.lock()->broadcastWS("isServerBusy", std::string(""), serverBusyBroadcast, nlohmann::json::basic_json());
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
								Doppelganger::Util::log(logContent.str(), "APICALL", room.lock()->dataDir_, room.lock()->logConfig_);
							}

							nlohmann::json response, broadcast;

							room.lock()->plugin_.at(APIName).pluginProcess(
								core,
								room,
								parameters.at("parameters"),
								response,
								broadcast);

							{
								nlohmann::json serverBusyBroadcast = nlohmann::json::object();
								serverBusyBroadcast["isBusy"] = false;
								room.lock()->broadcastWS("isServerBusy", std::string(""), serverBusyBroadcast, nlohmann::json::basic_json());
							}

							// broadcast
							if (!broadcast.is_null())
							{
								room.lock()->broadcastWS(APIName, std::string(""), broadcast, response);
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
					std::string completeURL("");
					{
						completeURL += core.lock()->serverConfig_.protocol;
						completeURL += "://";
						completeURL += core.lock()->serverConfig_.host;
						completeURL += ":";
						completeURL += std::to_string(core.lock()->serverConfig_.portUsed);
					}

					std::string location = completeURL;
					location += "/";
					location += room.lock()->UUID_;
					location += "/html/index.html";
					return send(movedPermanently(std::move(req), location));
				}
			}
			else
			{
				// return 301 (moved permanently)
				std::string completeURL("");
				{
					completeURL += core.lock()->serverConfig_.protocol;
					completeURL += "://";
					completeURL += core.lock()->serverConfig_.host;
					completeURL += ":";
					completeURL += std::to_string(core.lock()->serverConfig_.portUsed);
				}

				std::string location = completeURL;
				location += "/";
				location += room.lock()->UUID_;
				location += "/html/index.html";
				return send(movedPermanently(std::move(req), location));
			}
		}
		else
		{
			// return 301 (moved permanently)
			std::string completeURL("");
			{
				completeURL += core.lock()->serverConfig_.protocol;
				completeURL += "://";
				completeURL += core.lock()->serverConfig_.host;
				completeURL += ":";
				completeURL += std::to_string(core.lock()->serverConfig_.portUsed);
			}

			std::string location = completeURL;
			location += "/";
			location += room.lock()->UUID_;
			location += "/html/index.html";
			return send(movedPermanently(std::move(req), location));
		}
	}
}

#endif