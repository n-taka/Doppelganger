#ifndef ROOM_CPP
#define ROOM_CPP

#include "Doppelganger/Room.h"
#include "Doppelganger/Core.h"
#include "Doppelganger/WebsocketSession.h"

namespace Doppelganger
{
	Room::Room(const std::string &UUID_,
			   const std::shared_ptr<Core> &core_)
		: UUID(UUID_), core(core_)
	{
		//////
		// initialize edit history parameters
		{
			// todo
			// editHistoryParams.editHistoryVec.clear();
			// editHistoryParams.editHistoryVec.push_back(editHistory({std::vector<std::string>({}), nlohmann::json::object()}));
			// editHistoryParams.editHistoryIndex = 0;
			// editHistoryParams.mutexEditHistoryParams = std::make_shared<std::mutex>();
		}

		//////
		// initialize user interface parameters
		{
			interfaceParams.cameraTarget << 0, 0, 0;
			interfaceParams.cameraPos << -30, 40, 30;
			interfaceParams.cameraUp << 0, 1, 0;
			interfaceParams.cameraZoom = 1.0;
		}

		////
		// initialize logger for this room
		{
			logger.initialize(UUID, core->config.at("log"));
			{
				std::stringstream ss;
				ss << "New room \"" << UUID << "\" is created.";
				logger.log(ss.str(), "SYSTEM");
			}
		}

		////
		// initialize outputDir for this room
		{
			const std::string roomCreatedTimeStamp = Logger::getCurrentTimestampAsString(false);
			std::stringstream tmp;
			tmp << roomCreatedTimeStamp;
			tmp << "-";
			tmp << UUID;
			const std::string timestampAndUUID = tmp.str();

			outputDir = core->config.at("output").at("dir").get<std::string>();
			outputDir.append(timestampAndUUID);
			outputDir.make_preferred();
			fs::create_directories(outputDir);
		}
	}

	void Room::joinWS(const std::shared_ptr<WebsocketSession> &session)
	{
		std::lock_guard<std::mutex> lock(serverParams.mutexServerParams);
		serverParams.websocketSessions[session->UUID] = session;
	}

	void Room::leaveWS(const std::string &sessionUUID)
	{
		std::lock_guard<std::mutex> lock(serverParams.mutexServerParams);
		serverParams.websocketSessions.erase(sessionUUID);
		// todo
		// update cursors
	}

	void Room::broadcastWS(const std::string &payload, const std::unordered_set<std::string> &doNotSend)
	{
		std::lock_guard<std::mutex> lock(serverParams.mutexServerParams);
		const std::shared_ptr<const std::string> ss = std::make_shared<const std::string>(std::move(payload));

		for (const auto &uuid_session : serverParams.websocketSessions)
		{
			if (doNotSend.find(uuid_session.first) == doNotSend.end())
			{
				uuid_session.second->send(ss);
			}
		}
	}

#if 0
	void Room::broadcastMeshUpdate(const std::vector<int> &doppelIdVec)
	{
		// load internal API
		typedef std::function<void(const int &, const std::unordered_map<int, Doppel::triangleMesh> &, nlohmann::json &, const bool)> writeMeshToJson_t;
		const writeMeshToJson_t &writeMeshToJson = boost::any_cast<writeMeshToJson_t &>(internalAPI.at("writeMeshToJson"));

		nlohmann::json meshesJson = nlohmann::json::object();
		for (const auto &doppelId : doppelIdVec)
		{
			nlohmann::json meshJson = nlohmann::json::object();
			writeMeshToJson(doppelId, meshes, meshJson, true);
			meshesJson[std::to_string(doppelId)] = meshJson;
		}
		nlohmann::json json = nlohmann::json::object();
		json["task"] = "syncMeshes";
		json["meshes"] = meshesJson;

		broadcastWS(json.dump());
	}
#endif
} // namespace

#endif
