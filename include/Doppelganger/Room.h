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

		void setup(
			const std::string &UUID,
			nlohmann::json &configCore);

		////
		// nlohmann::json conversion (we do NOT use implicit conversions)
		////
		void to_json(nlohmann::json &json) const;
		void from_json(const nlohmann::json &json);
		void applyCurrentConfig();
		// void storeHistory(const nlohmann::json &diff, const nlohmann::json &diffInv);

		void joinWS(const WSSession &session);
		void leaveWS(const std::string &sessionUUID);
		void broadcastWS(const std::string &APIName, const std::string &sourceUUID, const nlohmann::json &broadcast, const nlohmann::json &response);

	public:
		////
		// parameters stored in nlohmann::json
		struct PluginInfo
		{
			std::string name;
			std::string version;
		};
		struct History
		{
			int index;
			// for changing i => i+1 (redo),
			//   we simply apply diffFromPrev.at(i+1)
			std::vector<nlohmann::json> diffFromPrev;
			// for changing i+1 => i (undo),
			//   we simply apply diffFromNext.at(i)
			//   diffFromNext is automatically calculated within storeCurrent(...)
			std::vector<nlohmann::json> diffFromNext;
		};
		bool active_;
		// room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
		std::string UUID_;
		// Doppelganger
		fs::path DoppelgangerRootDir_;
		Util::LogConfig logConfig_;
		std::string outputType_;
		std::vector<Plugin::InstalledVersionInfo> installedPlugin_;
		std::vector<std::string> pluginListURL_;
		std::unordered_map<std::string, Doppelganger::TriangleMesh> meshes_;
		History history_;
		nlohmann::json extension_;

		////
		// parameters **NOT** stored in nlohmann::json
		// Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/
		fs::path dataDir_;
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

////
// edit history
////
// edit history is defined as a json object
// {
//     "meshUUID-A": json object represents mesh with meshUUID-A,
//     "meshUUID-B": json object represents mesh with meshUUID-B,
//     "meshUUID-C": {
//         remove: true
//     },
//     ...
// }
// * each json object ALWAYS contains entry with a key "remove"

#endif
