#ifndef ROOM_CPP
#define ROOM_CPP

#include "Doppelganger/Room.h"
#include "Doppelganger/Core.h"
#include "Doppelganger/Plugin.h"
#include "Doppelganger/WebsocketSession.h"
#include "Doppelganger/Util/getCurrentTimestampAsString.h"

#include <fstream>
#include <sstream>

namespace Doppelganger
{
	Room::Room(const std::string &UUID,
			   const std::shared_ptr<Core> &core)
		: UUID_(UUID), core_(core)
	{
	}

	void Room::setup()
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
			interfaceParams.cameraPosition << -30, 40, 30;
			interfaceParams.cameraUp << 0, 1, 0;
			interfaceParams.cameraZoom = 1.0;
			interfaceParams.cameraTargetTimestamp = 0;
			interfaceParams.cameraPositionTimestamp = 0;
			interfaceParams.cameraUpTimestamp = 0;
			interfaceParams.cameraZoomTimestamp = 0;
		}

		////
		// directory for Room
		// Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/
		{
			dataDir = core_->DoppelgangerRootDir;
			dataDir.append("data");
			std::string dirName("");
			dirName += Util::getCurrentTimestampAsString(false);
			dirName += "-";
			dirName += UUID_;
			dataDir.append(dirName);
			fs::create_directories(dataDir);
		}

		////
		// log
		// Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/log
		if (core_->config.contains("log"))
		{
			nlohmann::json &logJson = core_->config.at("log");
			logger.initialize(dataDir, logJson);

			{
				std::stringstream ss;
				ss << "New room \"" << UUID_ << "\" is created.";
				logger.log(ss.str(), "SYSTEM");
			}
		}

		////
		// output
		// Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/output
		if (core_->config.contains("output"))
		{
			nlohmann::json &outputJson = core_->config.at("output");
			// output["type"] == "storage"|"download"
			fs::path outputDir = dataDir;
			outputDir.append("output");
			fs::create_directories(outputDir);
		}

		////
		// plugin
		if (core_->config.contains("plugin"))
		{
			// install plugins
			fs::path installedPluginJsonPath(dataDir);
			installedPluginJsonPath.append("installed.json");
			if (!fs::exists(installedPluginJsonPath))
			{
				fs::path coreInstalledPluginJsonPath(core_->DoppelgangerRootDir);
				coreInstalledPluginJsonPath.append("plugin");
				coreInstalledPluginJsonPath.append("installed.json");
				fs::copy_file(coreInstalledPluginJsonPath, installedPluginJsonPath);
				plugin = core_->plugin;
			}

			{
				std::ifstream ifs(installedPluginJsonPath.string());
				const nlohmann::json installedPluginJson = nlohmann::json::parse(ifs);
				ifs.close();

				// we erase previous installed plugin array.
				// following Doppelganger::Plugin::install updates intalled.json
				const nlohmann::json emptyArray = nlohmann::json::array();
				std::ofstream ofs(installedPluginJsonPath.string());
				ofs << emptyArray.dump(4);
				ofs.close();

				for (const auto &pluginToBeInstalled : installedPluginJson)
				{
					const std::string &name = pluginToBeInstalled.at("name").get<std::string>();
					const std::string &version = pluginToBeInstalled.at("version").get<std::string>();

					if (plugin.find(name) != plugin.end() && version.size() > 0)
					{
						plugin.at(name)->install(shared_from_this(), version);
						std::cout << "install room" << std::endl;
					}
					else
					{
						std::stringstream ss;
						ss << "Plugin \"" << name << "\" (" << version << ")"
						   << " is NOT found.";
						logger.log(ss.str(), "ERROR");
					}
				}
			}
		}
	}

	void Room::joinWS(const std::variant<std::shared_ptr<PlainWebsocketSession>, std::shared_ptr<SSLWebsocketSession>> &session)
	{
		std::lock_guard<std::mutex> lock(serverParams.mutex);
		std::visit([&](const auto &session_)
				   { serverParams.websocketSessions[session_->UUID_] = session_; },
				   session);
	}

	void Room::leaveWS(const std::string &sessionUUID)
	{
		std::lock_guard<std::mutex> lock(serverParams.mutex);
		serverParams.websocketSessions.erase(sessionUUID);
		// todo
		// update cursors
	}

	void Room::broadcastWS(const std::string &APIName, const std::string &sourceUUID, const nlohmann::json &broadcast, const nlohmann::json &response)
	{
		std::lock_guard<std::mutex> lock(serverParams.mutex);
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
			const std::variant<std::shared_ptr<PlainWebsocketSession>, std::shared_ptr<SSLWebsocketSession>> &session = uuid_session.second;
			if (sessionUUID != sourceUUID)
			{
				if (!broadcast.empty())
				{
					std::visit([&](const auto &session_)
							   { session_->send(broadcastMessage); },
							   session);
				}
			}
			else
			{
				if (!response.empty())
				{
					std::visit([&](const auto &session_)
							   { session_->send(responseMessage); },
							   session);
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
} // namespace

#endif
