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
	Room::Room(const std::string &UUID)
		: UUID_(UUID)
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
		// config.json
		nlohmann::json config;
		{
			std::lock_guard<std::mutex> lock(mutexConfig);
			getCurrentConfig(config);
		}

		////
		// directory for Room
		// Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/
		{
			dataDir = Core::getInstance().DoppelgangerRootDir;
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
		if (config.contains("log"))
		{
			logger.initialize(shared_from_this());
			{
				std::stringstream ss;
				ss << "New room \"" << UUID_ << "\" is created.";
				logger.log(ss.str(), "SYSTEM");
			}
		}

		////
		// output
		// Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/output
		if (config.contains("output"))
		{
			fs::path outputDir = dataDir;
			outputDir.append("output");
			fs::create_directories(outputDir);
		}

		////
		// plugin
		// TODO!!!!
		if (config.contains("plugin"))
		{
			// get plugin catalogue
			nlohmann::json pluginCatalogue;
			Plugin::getPluginCatalogue(config.at("plugin").at("listURL"), pluginCatalogue);

			// initialize Doppelganger::Plugin instances
			for (const auto &pluginEntry : pluginCatalogue.items())
			{
				const std::string &name = pluginEntry.key();
				const nlohmann::json &pluginInfo = pluginEntry.value();
				plugin[name] = std::make_shared<Doppelganger::Plugin>(name, pluginInfo);
			}

			// install plugins
			{
				const nlohmann::json installedPluginJson = config.at("plugin").at("installed");
				config.at("plugin").at("installed") = nlohmann::json::array();

				for (const auto &pluginToBeInstalled : installedPluginJson)
				{
					const std::string &name = pluginToBeInstalled.at("name").get<std::string>();
					const std::string &version = pluginToBeInstalled.at("version").get<std::string>();

					if (plugin.find(name) != plugin.end() && version.size() > 0)
					{
						plugin.at(name)->install(shared_from_this(), version);
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

			// install non-optional plugins
			for (const auto &pluginEntry : pluginCatalogue.items())
			{
				const std::string &name = pluginEntry.key();
				const nlohmann::json &pluginInfo = pluginEntry.value();
				const bool &optional = pluginInfo.at("optional").get<bool>();
				if (!optional)
				{
					if (plugin.find(name) != plugin.end())
					{
						if (plugin.at(name)->installedVersion.size() == 0)
						{
							plugin.at(name)->install(shared_from_this(), std::string("latest"));
						}
					}
					else
					{
						std::stringstream ss;
						ss << "Non-optional plugin \"" << name << "\" (latest)"
						   << " is NOT found.";
						logger.log(ss.str(), "ERROR");
					}
				}
			}
		}
	}

	void Room::joinWS(const std::variant<std::shared_ptr<PlainWebsocketSession>, std::shared_ptr<SSLWebsocketSession>> &session)
	{
		std::lock_guard<std::mutex> lock(mutexWS);
		std::visit([&](const auto &session_)
				   { websocketSessions[session_->UUID_] = session_; },
				   session);
	}

	void Room::leaveWS(const std::string &sessionUUID)
	{
		std::lock_guard<std::mutex> lock(mutexWS);
		websocketSessions.erase(sessionUUID);
		// todo
		// update cursors
	}

	void Room::broadcastWS(const std::string &APIName, const std::string &sourceUUID, const nlohmann::json &broadcast, const nlohmann::json &response)
	{
		std::lock_guard<std::mutex> lock(mutexWS);
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

		for (const auto &uuid_session : websocketSessions)
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

	void Room::getCurrentConfig(nlohmann::json &config) const
	{
		fs::path configPath(dataDir);
		configPath.append("config.json");
		if (fs::exists(configPath))
		{
			// we could directly use std::filesystem::path, but we could not directly boost::filesystem::path
			std::ifstream ifs(configPath.string());
			config = nlohmann::json::parse(ifs);
			ifs.close();
		}
		else
		{
			// no config file is found.
			// we copy from core
			std::cout << "No config file for Room found. We copy from Core." << std::endl;
			{
				std::lock_guard<std::mutex> lock(Core::getInstance().mutexConfig);
				Core::getInstance().getCurrentConfig(config);
			}
			updateConfig(config);
		}
	}

	void Room::updateConfig(const nlohmann::json &config) const
	{
		fs::path configPath(dataDir);
		configPath.append("config.json");
		std::ofstream ofs(configPath.string());
		ofs << config.dump(4);
		ofs.close();
	}
} // namespace

#endif
