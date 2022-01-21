#ifndef ROOM_H
#define ROOM_H

#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
// todo variant requires C++17
// we need to use boost::variant for macOS
#include <variant>

#include <nlohmann/json.hpp>

#include "Doppelganger/TriangleMesh.h"

namespace Doppelganger
{
	class Plugin;
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

		void applyCurrentConfig();

		void joinWS(const WSSession &session);
		void leaveWS(const std::string &sessionUUID);
		void broadcastWS(const std::string &APIName, const std::string &sourceUUID, const nlohmann::json &broadcast, const nlohmann::json &response);

	public:
		nlohmann::json config_;

		// mesh data
		std::unordered_map<std::string, std::shared_ptr<Doppelganger::TriangleMesh>> meshes_;

		// plugins
		std::unordered_map<std::string, std::unique_ptr<Doppelganger::Plugin>> plugin_;

		// websockets
		std::unordered_map<std::string, WSSession> websocketSessions_;
		std::mutex mutexWS_;

		nlohmann::json extension_;
	};

	////
	// nlohmann::json conversion (but, we do NOT use implicit conversions, hehe)
	////
	void to_json(nlohmann::json &json, const Room &room);
	void from_json(const nlohmann::json &json, Room &room);
}

////
// json format for Doppelganger::Room
////
// {
//   "extension": json for extension (used by plugins)
// }

// // Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/
// fs::path dataDir;

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
//  "meshes"
//  "meshUUID-A": json object represents mesh with meshUUID-A,
//  "meshUUID-B": json object represents mesh with meshUUID-B,
//  "meshUUID-C": {
//	 remove: true
//  },
//  ...
// }
// * each json object ALWAYS contains entry with a key "remove"
// struct EditHistory
// {
// 	int index;
// 	// for changing i => i+1 (redo),
// 	//   we simply apply diffFromPrev.at(i+1)
// 	std::vector<nlohmann::json> diffFromPrev;
// 	// for changing i+1 => i (undo),
// 	//   we simply apply diffFromNext.at(i)
// 	//   diffFromNext is automatically calculated within storeCurrent(...)
// 	std::vector<nlohmann::json> diffFromNext;
// };
// EditHistory editHistory;
// void storeHistory(const nlohmann::json &diff, const nlohmann::json &diffInv);

#endif
