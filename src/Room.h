#ifndef ROOM_H
#define ROOM_H

#include "triangleMesh.h"

#include <string>
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

namespace Doppelganger
{
	class WebsocketSession;

	class Room
	{
	public:
		Room(const std::string &UUID_);
		~Room() {}

	public:
		std::string UUID;

		////
		// parameters for server setup
		struct serverParameters
		{
			std::unordered_map<std::string, std::shared_ptr<WebsocketSession>> websocketSessions;
			std::shared_ptr<std::mutex> mutexServerParams;
		};
		serverParameters serverParams;
		int joinWS(const std::shared_ptr<WebsocketSession> &session);
		void leaveWS(const std::string &sessionUUID);
		void broadcastWS(const std::string &payload, const std::unordered_set<int> &exceptionSet = {});
		void broadcastMeshUpdate(const std::vector<std::string> &meshUUIDVec);

		////
		// parameter for edit history
		struct editHistory
		{
			std::vector<std::string> updatedFromPrevUUIDVec;
			nlohmann::json json;
		};
		struct editHistoryParameters
		{
			int editHistoryIndex;
			std::vector<editHistory> editHistoryVec;
			std::shared_ptr<std::mutex> mutexEditHistoryParams;
		};
		editHistoryParameters editHistoryParams;

		////
		// parameter for user interface
		struct interfaceParameters
		{
			Eigen::Matrix<double, 3, 1> cameraTarget, cameraPos, cameraUp;
			double cameraZoom;
			// mouse cursors
			std::unordered_map<std::string, std::vector<double>> cursors;
			// loading state
			std::unordered_set<std::string> taskUUIDInProgress;
			std::shared_ptr<std::mutex> mutexInterfaceParams;
		};
		interfaceParameters interfaceParams;

		////
		// mesh data
		std::unordered_map<std::string, Doppelganger::triangleMesh> meshes;
		std::shared_ptr<std::mutex> mutexMeshes;

		////
		// custom data
		std::unordered_map<std::string, boost::any> customData;
	};
} // namespace Doppel

#endif
