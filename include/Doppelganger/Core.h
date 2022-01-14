#ifndef CORE_H
#define CORE_H

#if defined(_WIN64)
#ifdef DLL_EXPORT
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif
#elif defined(__APPLE__)
#define DECLSPEC
#elif defined(__linux__)
#define DECLSPEC
#endif

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

#include <memory>
#include <string>
#include <unordered_map>

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

#include <nlohmann/json.hpp>
#include "Doppelganger/Logger.h"

namespace Doppelganger
{
	class Room;
	class Plugin;
	class triangleMesh;
	class Listener;

	class DECLSPEC Core : public std::enable_shared_from_this<Core>
	{
	public:
		Core(boost::asio::io_context &ioc,
			 boost::asio::ssl::context &ctx);

		void setup();
		void run();

		nlohmann::json configOrig, config;
		Logger logger;
		std::unordered_map<std::string, std::shared_ptr<Doppelganger::Room>> rooms;
		std::unordered_map<std::string, std::shared_ptr<Doppelganger::Plugin>> plugin;
		// Doppelganger/
		fs::path DoppelgangerRootDir;
		// Doppelganger/data/YYYYMMDDTHHMMSS-Core/
		fs::path dataDir;

		// for graceful shutdown
		boost::asio::io_context &ioc_;
		boost::asio::ssl::context &ctx_;

	private:
		std::shared_ptr<Listener> listener;
	};
}

#endif
