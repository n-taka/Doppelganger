#ifndef CORE_H
#define CORE_H

#include <memory>
#include <string>
#include <unordered_map>
#if defined(_WIN32) || defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#endif
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

namespace Doppelganger
{
	class Room;

	class Core : public std::enable_shared_from_this<Core>
	{
	public:
		Core(boost::asio::io_context &ioc_);

		void setup();
		void run();

		////
		// parameters for Doppel
		struct systemParameters
		{
			fs::path baseDir;
			std::mutex mutexSystemParams;
		};
		systemParameters systemParams;

		// 		////
		// 		// API
		// 		// typedef for REST API
		// 		typedef std::function<void(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &, nlohmann::json &)> RESTAPI_t;
		// 		typedef std::unordered_map<std::string, std::unordered_map<std::string, RESTAPI_t> > RESTAPIMAP_t;
		// #if defined(_WIN32) || defined(_WIN64)
		// 		typedef void(__stdcall *RESTAPIPtr_t)(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &, nlohmann::json &);
		// #else
		// 		typedef void (*RESTAPIPtr_t)(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &, nlohmann::json &);
		// #endif
		// 		// typedef for WS API
		// 		typedef std::function<void(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &)> WSAPI_t;
		// 		typedef std::unordered_map<std::string, WSAPI_t> WSAPIMAP_t;
		// #if defined(_WIN32) || defined(_WIN64)
		// 		typedef void(__stdcall *WSAPIPtr_t)(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &);
		// #else
		// 		typedef void (*WSAPIPtr_t)(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &);
		// #endif

		// 		std::unordered_map<std::string, boost::any> API;
		nlohmann::json config;
		std::unordered_map<std::string, std::shared_ptr<Doppelganger::Room>> rooms;

	private:
		// for graceful shutdown
		boost::asio::io_context &ioc;
		boost::asio::ip::tcp::acceptor acceptor;
		boost::asio::ip::tcp::socket socket;

		void parseConfig(
			const fs::path &pathConfig,
			const fs::path &baseDir,
			nlohmann::json &config);

		void fail(
			boost::system::error_code ec,
			char const *what);
		void onAccept(
			boost::system::error_code ec);
	};
}

#endif
