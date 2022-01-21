#ifndef CORE_H
#define CORE_H

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
#include <mutex>

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

namespace Doppelganger
{
	class Room;
	class Plugin;
	class Listener;

	class Core : public std::enable_shared_from_this<Core>
	{
	public:
		Core(boost::asio::io_context &ioc,
			 boost::asio::ssl::context &ctx);

		void setup();
		void run();

		void applyCurrentConfig();
		void storeCurrentConfig() const;

	public:
		// config for Core
		nlohmann::json config_;
		// rooms
		std::unordered_map<std::string, std::shared_ptr<Doppelganger::Room>> rooms_;

		// plugins (maybe this member variable is almost useless...)
		std::unordered_map<std::string, std::unique_ptr<Doppelganger::Plugin>> plugin_;

		// boost::asio
		boost::asio::io_context &ioc_;
		boost::asio::ssl::context &ctx_;

	private:
		void loadServerCertificate(const fs::path &certificatePath, const fs::path &privateKeyPath);
		static void generateDefaultConfigJson(nlohmann::json &config);
		std::shared_ptr<Listener> listener_;
	};
}

#endif
