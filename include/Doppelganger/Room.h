#ifndef ROOM_H
#define ROOM_H

#if defined(_WIN64)
#include <filesystem>
#include <variant>
namespace fs = std::filesystem;
template <class... Types>
using variant = std::variant<Types...>;
#elif defined(__APPLE__)
#include "boost/filesystem.hpp"
#include "boost/variant.hpp"
namespace fs = boost::filesystem;
template <class... Types>
using variant = boost::variant<Types...>;
#elif defined(__linux__)
#include <filesystem>
#include <variant>
namespace fs = std::filesystem;
template <class... Types>
using variant = std::variant<Types...>;
#endif

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include <nlohmann/json.hpp>
#include "Doppelganger/Plugin.h"
#include "Doppelganger/Util/log.h"

namespace Doppelganger
{
	class PlainWebsocketSession;
	class SSLWebsocketSession;

	class Room : public std::enable_shared_from_this<Room>
	{
		using WSSession = variant<std::shared_ptr<PlainWebsocketSession>, std::shared_ptr<SSLWebsocketSession> >;

	public:
		Room();

		// we explicitly copy configCore
		void setup(
			const std::string &UUID,
			const nlohmann::json &configCore);

		////
		// nlohmann::json conversion (we do NOT use implicit conversions)
		////
		// void to_json(nlohmann::json &json) const;
		// void from_json(const nlohmann::json &json);
		void applyCurrentConfig();

		void joinWS(const WSSession &session);
		void leaveWS(const std::string &sessionUUID);
		void broadcastWS(const std::string &APIName, const std::string &sourceUUID, const nlohmann::json &broadcast, const nlohmann::json &response);

	public:
		nlohmann::json config;

		////
		// parameters **NOT** stored in nlohmann::json
		// Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/
		std::unordered_map<std::string, Doppelganger::Plugin> plugin_;

		std::mutex mutexRoom_;
		std::unordered_map<std::string, WSSession> websocketSessions_;
		std::mutex mutexWS_;
	};
}

#endif
