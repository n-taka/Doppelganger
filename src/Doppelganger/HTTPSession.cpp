#ifndef HTTPSESSION_CPP
#define HTTPSESSION_CPP

#include "Doppelganger/HTTPSession.h"

#include "Doppelganger/Core.h"
#include "Doppelganger/Logger.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/WebsocketSession.h"
#include "Doppelganger/Plugin.h"
#include "Util/uuid.h"

#if defined(_WIN32) || defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#endif

namespace
{
	boost::beast::string_view
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
	std::string parseRoomUUID(const boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &req)
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
	boost::beast::http::response<boost::beast::http::string_body> badRequest(
		boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req,
		boost::beast::string_view why)
	{
		// Returns a bad request response
		boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::bad_request, req.version()};
		res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(boost::beast::http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = why.to_string();
		res.prepare_payload();
		return res;
	}

	template <class Body, class Allocator>
	boost::beast::http::response<boost::beast::http::string_body> notFound(
		boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req,
		boost::beast::string_view target)
	{
		// Returns a not found response
		boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::not_found, req.version()};
		res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(boost::beast::http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = "The resource '" + target.to_string() + "' was not found.";
		res.prepare_payload();
		return res;
	}

	template <class Body, class Allocator>
	boost::beast::http::response<boost::beast::http::string_body> serverError(
		boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req,
		boost::beast::string_view what)
	{
		// Returns a server error response
		boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::internal_server_error, req.version()};
		res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(boost::beast::http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = "An error occurred: '" + what.to_string() + "'";
		res.prepare_payload();
		return res;
	}

	template <class Body, class Allocator>
	boost::beast::http::response<boost::beast::http::string_body> movedPermanently(
		boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req,
		boost::beast::string_view location)
	{
		// Returns a moved permanently
		boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::moved_permanently, req.version()};
		res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(boost::beast::http::field::content_type, "text/html");
		res.set(boost::beast::http::field::location, location);
		res.keep_alive(req.keep_alive());
		res.prepare_payload();
		return res;
	}

	template <class Body, class Allocator, class Send>
	void handleRequest(const std::shared_ptr<Doppelganger::Core> &core,
					   const std::shared_ptr<Doppelganger::Room> &room,
					   boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req,
					   Send &&send)
	{
		// Make sure we can handle the method
		if (req.method() != boost::beast::http::verb::head &&
			req.method() != boost::beast::http::verb::get &&
			req.method() != boost::beast::http::verb::post)
		{
			return send(badRequest(std::move(req), "Unknown HTTP-method"));
		}

		// Request path must be absolute and not contain "..".
		if (req.target().empty() ||
			req.target()[0] != '/' ||
			req.target().find("..") != boost::beast::string_view::npos)
		{
			return send(badRequest(std::move(req), "Illegal request-target"));
		}

		// resource
		// http://example.com/<roomUUID>/<path>/<to>/<resource>
		// API
		// http://example.com/<roomUUID>/<APIName>
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
						fs::path completePath(core->systemParams.resourceDir);
						completePath.make_preferred();
						for (int pIdx = 2; pIdx < reqPathVec.size(); ++pIdx)
						{
							completePath.append(reqPathVec.at(pIdx));
						}

						// // note: second condition is for boost::filesystem
						// if (!completePath.has_filename() || completePath.filename().string() == ".")
						// {
						// 	// moved permanently (301)
						// 	std::string location = core->config.at("completeURL").get<std::string>();
						// 	location += "/html/index.html";

						// 	return send(movedPermanently(res, location));
						// }

						// Attempt to open the file
						boost::beast::error_code ec;
						boost::beast::http::file_body::value_type body;
						body.open(completePath.string().c_str(), boost::beast::file_mode::scan, ec);

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

						// Respond to HEAD request
						if (req.method() == boost::beast::http::verb::head)
						{
							boost::beast::http::response<boost::beast::http::empty_body> res{boost::beast::http::status::ok, req.version()};
							res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
							res.set(boost::beast::http::field::content_type, mime_type(completePath));
							res.content_length(size);
							res.keep_alive(req.keep_alive());
							return send(std::move(res));
						}
						else if (req.method() == boost::beast::http::verb::get)
						{
							// Respond to GET request
							boost::beast::http::response<boost::beast::http::file_body> res{
								std::piecewise_construct,
								std::make_tuple(std::move(body)),
								std::make_tuple(boost::beast::http::status::ok, req.version())};
							res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
							res.set(boost::beast::http::field::content_type, mime_type(completePath));
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
								std::lock_guard<std::mutex> lock(room->interfaceParams.mutexInterfaceParams);
								taskUUID = Doppelganger::Util::uuid("task-");
								room->interfaceParams.taskUUIDInProgress.insert(taskUUID);

								nlohmann::json json = nlohmann::json::object();
								json["task"] = "isServerBusy";
								nlohmann::json parameters = nlohmann::json::object();
								parameters["isBusy"] = (room->interfaceParams.taskUUIDInProgress.size() > 0);
								json["parameters"] = parameters;
								room->broadcastWS(json.dump());
							}

							const std::string &APIName = reqPathVec.at(2);
							const Doppelganger::Plugin::API_t &APIFunc = core->plugin.at(APIName)->func;

							nlohmann::json parameters = nlohmann::json::object();
							boost::optional<std::uint64_t> size = req.payload_size();
							if (size && *size > 0)
							{
								parameters = nlohmann::json::parse(req.body());
							}
							std::stringstream logContent;
							logContent << req.method_string();
							logContent << " ";
							logContent << req.target().to_string();
							logContent << " ";
							logContent << "(";
							logContent << parameters.at("sessionUUID");
							logContent << ")";
							room->logger.log(logContent.str(), "APICALL");

							nlohmann::json response, broadcast;
							APIFunc(room, parameters, response, broadcast);

							{
								std::lock_guard<std::mutex> lock(room->interfaceParams.mutexInterfaceParams);
								room->interfaceParams.taskUUIDInProgress.erase(taskUUID);

								nlohmann::json json = nlohmann::json::object();
								json["task"] = "isServerBusy";
								nlohmann::json parameters = nlohmann::json::object();
								parameters["isBusy"] = (room->interfaceParams.taskUUIDInProgress.size() > 0);
								json["parameters"] = parameters;
								room->broadcastWS(json.dump());
							}

							// broadcast
							if (!broadcast.empty())
							{
								// todo
								// room->broadcast(broadcast);
							}

							// response
							const std::string responseStr = response.dump();
							boost::beast::http::string_body::value_type payloadBody = responseStr;
							boost::beast::http::response<boost::beast::http::string_body> res{
								std::piecewise_construct,
								std::make_tuple(std::move(payloadBody)),
								std::make_tuple(boost::beast::http::status::ok, req.version())};
							res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
							res.set(boost::beast::http::field::content_type, "application/json");
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
	HTTPSession::HTTPSession(boost::asio::ip::tcp::socket socket, const std::shared_ptr<Core> &core_)
		: socket(std::move(socket)), core(core_)
	{
	}

	void HTTPSession::run()
	{
		// Read a request
		boost::beast::http::async_read(socket, buffer, req,
									   [self = shared_from_this()](boost::system::error_code ec, std::size_t bytes)
									   {
										   self->onRead(ec, bytes);
									   });
	}

	// Report a failure
	void HTTPSession::fail(boost::system::error_code ec, char const *what)
	{
		// Don't report on canceled operations
		if (ec == boost::asio::error::operation_aborted)
		{
			return;
		}

		std::stringstream s;
		s << what << ": " << ec.message();
		core->logger.log(s.str(), "ERROR");
	}

	void HTTPSession::onRead(boost::system::error_code ec, std::size_t)
	{
		// This means they closed the connection
		if (ec == boost::beast::http::error::end_of_stream)
		{
			socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
			return;
		}

		// Handle the error, if any
		if (ec)
		{
			return fail(ec, "read (http)");
		}

		std::string roomUUID = parseRoomUUID(req);

		const auto send = [this](auto &&response)
		{
			// The lifetime of the message has to extend
			// for the duration of the async operation so
			// we use a shared_ptr to manage it.
			using response_type = typename std::decay<decltype(response)>::type;
			auto sp = std::make_shared<response_type>(std::forward<decltype(response)>(response));

#if 0
            // NOTE This causes an ICE in gcc 7.3
            // Write the response
            http::async_write(this->socket_, *sp,
				[self = shared_from_this(), sp](
					error_code ec, std::size_t bytes)
				{
					self->on_write(ec, bytes, sp->need_eof()); 
				});
#else
			// Write the response
			auto self = shared_from_this();
			boost::beast::http::async_write(this->socket, *sp,
											[self, sp](
												boost::system::error_code ec, std::size_t bytes)
											{
												self->onWrite(ec, bytes, sp->need_eof());
											});
#endif
		};

		{
			std::stringstream s;
			s << "Request received: \"" << req.target().to_string() << "\"";
			core->logger.log(s.str(), "SYSTEM");
		}

		if (roomUUID == "favicon.ico")
		{
			// do nothing
			// TODO 2021.08.17 prepare favion.ico
		}
		else if (roomUUID.size() <= 0 || core->rooms.find(roomUUID) == core->rooms.end())
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
			const std::shared_ptr<Room> room = std::make_shared<Room>(roomUUID, core);
			core->rooms[roomUUID] = std::move(room);

			// return 301 (moved permanently)
			std::string location = core->config.at("server").at("completeURL").get<std::string>();
			location += "/";
			location += roomUUID;

			return send(movedPermanently(std::move(req), location));
		}
		else
		{
			const std::shared_ptr<Room> &room = core->rooms.at(roomUUID);
			// See if it is a WebSocket Upgrade
			if (boost::beast::websocket::is_upgrade(req))
			{
				// Create a WebSocket session by transferring the socket
				const std::string sessionUUID = Util::uuid("session-");
				std::make_shared<WebsocketSession>(std::move(socket), sessionUUID, room)->run(std::move(req));
				return;
			}
			else
			{
				// Send the response
				return handleRequest(core, room, std::move(req), send);
			}
		}
	}

	void HTTPSession::onWrite(boost::system::error_code ec, std::size_t, bool close)
	{
		// Handle the error, if any
		if (ec)
		{
			return fail(ec, "write (http)");
		}

		if (close)
		{
			// This means we should close the connection, usually because
			// the response indicated the "Connection: close" semantic.
			socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
			return;
		}

		// Clear contents of the request message,
		// otherwise the read behavior is undefined.
		req = {};

		// Read another request
		boost::beast::http::async_read(socket, buffer, req,
									   [self = shared_from_this()](boost::system::error_code ec, std::size_t bytes)
									   {
										   self->onRead(ec, bytes);
									   });
	}
}

#endif
