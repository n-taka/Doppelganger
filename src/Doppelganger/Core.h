#ifndef CORE_H
#define CORE_H

#include <memory>
#include <string>
#include <unordered_map>
#if defined(_WIN32) || defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#endif
#include <boost/asio.hpp>
#include <boost/any.hpp>
#include <nlohmann/json.hpp>
#include "Logger.h"

namespace Doppelganger
{
	class Room;
	class triangleMesh;
	class Plugin;

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
			// read only (static resources)
			fs::path resourceDir;
			// read write (logs, outputs, plugins)
			fs::path workingDir;
			std::mutex mutexSystemParams;
		};
		systemParameters systemParams;

		nlohmann::json config;
		Logger logger;
		std::unordered_map<std::string, std::shared_ptr<Doppelganger::Room>> rooms;
		std::unordered_map<std::string, std::shared_ptr<Doppelganger::Plugin>> plugin;

	private:
		// for graceful shutdown
		boost::asio::io_context &ioc;
		boost::asio::ip::tcp::acceptor acceptor;
		boost::asio::ip::tcp::socket socket;

		void parseConfig(
			const fs::path &pathConfig,
			const fs::path &workingDir,
			nlohmann::json &config);

		void fail(
			boost::system::error_code ec,
			char const *what);
		void onAccept(
			boost::system::error_code ec);
	};
}

#endif
