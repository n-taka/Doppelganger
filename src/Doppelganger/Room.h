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
		void broadcastWS(const std::string &payload, const std::unordered_set<std::string> &doNotSend = {});
		// void broadcastMeshUpdate(const std::vector<std::string> &meshUUIDVec);

		////
		// parameter for edit history
		struct editHistoryParameters
		{
			int editHistoryIndex;
			std::vector<nlohmann::json> diffFromPrev;
			std::vector<nlohmann::json> diffFromNext;
			std::mutex mutexEditHistoryParams;
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
			std::mutex mutexInterfaceParams;
		};
		interfaceParameters interfaceParams;

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
