#ifndef ROOM_H
#define ROOM_H

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

#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
// todo variant requires C++17
// we need to use boost::variant for macOS
#include <variant>
#include <boost/any.hpp>
#include <nlohmann/json.hpp>

#include "Doppelganger/triangleMesh.h"
#include "Doppelganger/Logger.h"

namespace Doppelganger
{
	class Core;
	class Plugin;
	class PlainWebsocketSession;
	class SSLWebsocketSession;

	class DECLSPEC Room : public std::enable_shared_from_this<Room>
	{
	public:
		Room(
			const std::string &UUID,
			const std::shared_ptr<Core> &core);
		~Room() {}

		void setup();

	public:
		const std::string UUID_;
		const std::shared_ptr<Core> core_;
		Logger logger;
		// Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/
		fs::path dataDir;
		std::unordered_map<std::string, std::shared_ptr<Doppelganger::Plugin>> plugin;

		////
		// parameters for server setup
		struct serverParameters
		{
			std::unordered_map<std::string, std::variant<std::shared_ptr<PlainWebsocketSession>, std::shared_ptr<SSLWebsocketSession>>> websocketSessions;
			std::mutex mutex;
		};
		serverParameters serverParams;
		void joinWS(const std::variant<std::shared_ptr<PlainWebsocketSession>, std::shared_ptr<SSLWebsocketSession>> &session);
		void leaveWS(const std::string &sessionUUID);
		void broadcastWS(const std::string &APIName, const std::string &sourceUUID, const nlohmann::json &broadcast, const nlohmann::json &response);

		////
		// parameter for user interface
		struct cursorInfo
		{
			double x;
			double y;
			int idx;
		};
		struct interfaceParameters
		{
			Eigen::Matrix<double, 3, 1> cameraTarget, cameraPosition, cameraUp;
			double cameraZoom;
			std::int64_t cameraTargetTimestamp, cameraPositionTimestamp, cameraUpTimestamp, cameraZoomTimestamp;
			// mouse cursors
			std::unordered_map<std::string, cursorInfo> cursors;
			// loading state
			std::unordered_set<std::string> taskUUIDInProgress;
			std::mutex mutex;
		};
		interfaceParameters interfaceParams;

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
		struct EditHistory
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
		EditHistory editHistory;
		void storeHistory(const nlohmann::json &diff, const nlohmann::json &diffInv);

		////
		// mesh data
		std::unordered_map<std::string, std::shared_ptr<Doppelganger::triangleMesh>> meshes;
		std::mutex mutexMeshes;

		////
		// custom data
		std::unordered_map<std::string, boost::any> customData;
	};
} // namespace

#endif
