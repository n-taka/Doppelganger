#ifndef ROOM_H
#define ROOM_H

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

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
// todo variant requires C++17
// we need to use boost::variant for macOS
#include <variant>

#include <nlohmann/json.hpp>
#include "Doppelganger/TriangleMesh.h"
#include "Doppelganger/Plugin.h"
#include "Doppelganger/Util/log.h"

namespace Doppelganger
{
	class PlainWebsocketSession;
	class SSLWebsocketSession;

	class Room : public std::enable_shared_from_this<Room>
	{
		using WSSession = std::variant<std::shared_ptr<PlainWebsocketSession>, std::shared_ptr<SSLWebsocketSession>>;

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

// ////
// // parameter for user interface
// struct cursorInfo
// {
// 	double x;
// 	double y;
// 	int idx;
// };
// struct interfaceParameters
// {
// 	Eigen::Matrix<double, 3, 1> cameraTarget, cameraPosition, cameraUp;
// 	double cameraZoom;
// 	std::int64_t cameraTargetTimestamp, cameraPositionTimestamp, cameraUpTimestamp, cameraZoomTimestamp;
// 	// mouse cursors
// 	std::unordered_map<std::string, cursorInfo> cursors;
// 	// loading state
// 	std::unordered_set<std::string> taskUUIDInProgress;
// 	std::mutex mutex;
// };
// interfaceParameters interfaceParams;

#endif
