#ifndef ROOM_H
#define ROOM_H

#include "Doppelganger/triangleMesh.h"

#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <boost/any.hpp>
#include <nlohmann/json.hpp>
#if defined(_WIN32) || defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#endif

#include "Doppelganger/Logger.h"

namespace Doppelganger
{
	class Core;
	class WebsocketSession;

	class Room
	{
	public:
		Room(
			const std::string &UUID_,
			const std::shared_ptr<Core> &core_);
		~Room() {}

	public:
		std::string UUID;
		Logger logger;
		fs::path outputDir;
		const std::shared_ptr<Core> core;

		////
		// parameters for server setup
		struct serverParameters
		{
			std::unordered_map<std::string, std::shared_ptr<WebsocketSession>> websocketSessions;
			std::mutex mutexServerParams;
		};
		serverParameters serverParams;
		void joinWS(const std::shared_ptr<WebsocketSession> &session);
		void leaveWS(const std::string &sessionUUID);
		void broadcastWS(const std::string &APIName, const std::string &sourceUUID, const nlohmann::json &broadcast, const nlohmann::json &response);
		// void broadcastMeshUpdate(const std::vector<std::string> &meshUUIDVec);

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
			std::int64_t timestamp;
			Eigen::Matrix<double, 3, 1> cameraTarget, cameraPos, cameraUp;
			double cameraZoom;
			// mouse cursors
			std::unordered_map<std::string, cursorInfo> cursors;
			// loading state
			std::unordered_set<std::string> taskUUIDInProgress;
			std::mutex mutexInterfaceParams;
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
