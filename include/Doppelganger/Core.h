#ifndef CORE_H
#define CORE_H

#if defined(_WIN32) || defined(_WIN64)
#ifdef DLL_EXPORT
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif
#else
#define DECLSPEC
#endif

#include <memory>
#include <string>
#include <unordered_map>
#if defined(__APPLE__)
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "Doppelganger/Logger.h"

namespace Doppelganger
{
	class Room;
	class triangleMesh;
	class Plugin;

	class DECLSPEC Core : public std::enable_shared_from_this<Core>
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
			std::mutex mutex;
		};
		systemParameters systemParams;

		nlohmann::json configFileContent, config;
		Logger logger;
		std::unordered_map<std::string, std::shared_ptr<Doppelganger::Room>> rooms;
		std::unordered_map<std::string, std::shared_ptr<Doppelganger::Plugin>> plugin;

		// for graceful shutdown
		boost::asio::io_context &ioc;
	private:
		boost::asio::ip::tcp::acceptor acceptor;
		boost::asio::ip::tcp::socket socket;

		void fail(
			boost::system::error_code ec,
			char const *what);
		void onAccept(
			boost::system::error_code ec);
	};
}

#endif
