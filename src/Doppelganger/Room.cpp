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
		editHistory.index = 0;
		{
			const nlohmann::json empty = nlohmann::json::object();
			editHistory.diffFromPrev.emplace_back(empty);
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
		if (core->config.at("output").at("type").get<std::string>() == "local")
		{
			const std::string roomCreatedTimeStamp = Logger::getCurrentTimestampAsString(false);
			std::stringstream tmp;
			tmp << roomCreatedTimeStamp;
			tmp << "-";
			tmp << UUID;
			const std::string timestampAndUUID = tmp.str();

			outputDir = core->config.at("output").at("local").at("dir").get<std::string>();
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

	void Room::broadcastWS(const std::string &APIName, const std::string &sourceUUID, const nlohmann::json &broadcast, const nlohmann::json &response)
	{
		std::lock_guard<std::mutex> lock(serverParams.mutexServerParams);
		nlohmann::json broadcastJson = nlohmann::json::object();
		nlohmann::json responseJson = nlohmann::json::object();
		if (!broadcast.empty())
		{
			broadcastJson["API"] = APIName;
			// broadcastJson["sessionUUID"] = sourceUUID;
			broadcastJson["parameters"] = broadcast;
		}
		if (!response.empty())
		{
			responseJson["API"] = APIName;
			// responseJson["sessionUUID"] = sourceUUID;
			responseJson["parameters"] = response;
		}
		const std::shared_ptr<const std::string> broadcastMessage = std::make_shared<const std::string>(broadcastJson.dump());
		const std::shared_ptr<const std::string> responseMessage = std::make_shared<const std::string>(responseJson.dump());

		for (const auto &uuid_session : serverParams.websocketSessions)
		{
			const std::string &sessionUUID = uuid_session.first;
			const std::shared_ptr<WebsocketSession> &session = uuid_session.second;
			if (sessionUUID != sourceUUID)
			{
				if (!broadcast.empty())
				{
					session->send(broadcastMessage);
				}
			}
			else
			{
				if (!response.empty())
				{
					session->send(responseMessage);
				}
			}
		}
	}

	void Room::storeHistory(const nlohmann::json &diff, const nlohmann::json &diffInv)
	{
		// erase future elements in the edit history
		if (editHistory.index + 0 < editHistory.diffFromNext.size())
		{
			editHistory.diffFromNext.erase(editHistory.diffFromNext.begin() + (editHistory.index + 0), editHistory.diffFromNext.end());
		}
		if (editHistory.index + 1 < editHistory.diffFromPrev.size())
		{
			editHistory.diffFromPrev.erase(editHistory.diffFromPrev.begin() + (editHistory.index + 1), editHistory.diffFromPrev.end());
		}
		// store diff/diffInv
		editHistory.diffFromNext.emplace_back(diffInv);
		editHistory.diffFromPrev.emplace_back(diff);
		editHistory.index = editHistory.diffFromNext.size();
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
