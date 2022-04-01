#ifndef CORE_H
#define CORE_H

#include "Doppelganger/Util/filesystem.h"

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
#include "Doppelganger/Plugin.h"
#include "Doppelganger/Util/log.h"

namespace Doppelganger
{
	class Room;
	class Listener;

	class Core : public std::enable_shared_from_this<Core>
	{
	public:
		Core(boost::asio::io_context &ioc,
			 boost::asio::ssl::context &ctx);

		void setup();
		void run();
		void shutdown();

		////
		// nlohmann::json conversion
		//     note: we don't allow implicit conversion for Core
		////
		// void to_json(nlohmann::json &json) const;
		// void to_json(nlohmann::json &json, const nlohmann::json &ptrStrArray) const;
		// void to_json(nlohmann::json &json, const nlohmann::json_pointer &ptr) const;
		// void from_json(const nlohmann::json &json);
		void applyCurrentConfig();
		void storeCurrentConfig() const;

	public:
		nlohmann::json config;

		////
		// parameters **NOT** stored in nlohmann::json
		std::unordered_map<std::string, std::shared_ptr<Doppelganger::Room>> rooms_;

	private:
		void loadServerCertificate(const fs::path &certificatePath, const fs::path &privateKeyPath);
		std::shared_ptr<Listener> listener_;

	private:
		boost::asio::io_context &ioc_;
		boost::asio::ssl::context &ctx_;
	};
}

#endif
