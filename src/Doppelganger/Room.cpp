#ifndef ROOM_CPP
#define ROOM_CPP

#include "Doppelganger/Room.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include "Doppelganger/Plugin.h"
#include "Doppelganger/WebsocketSession.h"
#include "Doppelganger/Util/getCurrentTimestampAsString.h"
#include "Doppelganger/Util/log.h"

namespace Doppelganger
{
	Room::Room()
	{
	}

	void Room::setup(
		const std::string &UUID,
		nlohmann::json &configCore)
	{
		// basically, we inherit configCore
		config_ = configCore;

		// meta info
		config_["UUID"] = UUID;
		config_["active"] = true;

		// wipe out the settings for Core
		config_.at("data").at("dir") = std::string("");
		config_.at("data").at("initialized") = false;

		// add Room specific parameters
		// interface
		// camera
		{
			config_["camera"] = nlohmann::json::object();
			config_["camera"]["target"] = nlohmann::json::object();
			config_["camera"]["target"]["x"] = 0.0;
			config_["camera"]["target"]["y"] = 0.0;
			config_["camera"]["target"]["z"] = 0.0;
			config_["camera"]["target"]["timestamp"] = 0;
			config_["camera"]["position"] = nlohmann::json::object();
			config_["camera"]["position"]["x"] = -30.0;
			config_["camera"]["position"]["y"] = 40.0;
			config_["camera"]["position"]["z"] = 30.0;
			config_["camera"]["position"]["timestamp"] = 0;
			config_["camera"]["up"] = nlohmann::json::object();
			config_["camera"]["up"]["x"] = 0.0;
			config_["camera"]["up"]["y"] = 1.0;
			config_["camera"]["up"]["z"] = 0.0;
			config_["camera"]["up"]["timestamp"] = 0;
			config_["camera"]["zoom"] = nlohmann::json::object();
			config_["camera"]["zoom"]["value"] = 1.0;
			config_["camera"]["zoom"]["timestamp"] = 0;
		}
		// cursor
		{
			config_["cursor"] = nlohmann::json::object();
		}

		// history
		{
			config_["history"] = nlohmann::json::object();
			config_["history"]["currentIndex"] = 0;
			config_["history"]["diffFromPrev"] = nlohmann::json::array();
			config_["history"]["diffFromNext"] = nlohmann::json::array();
			config_["history"]["diffFromPrev"].emplace_back(nlohmann::json::object());
		}

		// apply current config
		applyCurrentConfig();

		// log
		{
			std::stringstream ss;
			ss << "New room \"" << config_.at("UUID").get<std::string>() << "\" is created.";
			Util::log(ss.str(), "SYSTEM", config_);
		}
	}

	void Room::applyCurrentConfig()
	{
		if (!config_.at("active").get<bool>())
		{
			// shutdown...
			// TODO: close websocket sessions, etc...
			return;
		}

		// path for DoppelgangerRoot
		const fs::path DoppelgangerRootDir(config_.at("DoppelgangerRootDir").get<std::string>());

		// dataDir: Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/
		if (!config_.at("data").at("initialized").get<bool>())
		{
			fs::path dataDir(DoppelgangerRootDir);
			dataDir.append("data");
			std::string dirName("");
			dirName += Util::getCurrentTimestampAsString(false);
			dirName += "-";
			dirName += config_.at("UUID").get<std::string>();
			dataDir.append(dirName);
			fs::create_directories(dataDir);
			config_.at("data").at("dir") = dataDir.string();
			config_.at("data").at("initialized") = true;
		}

		// log: Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/log
		{
			fs::path logDir(config_.at("data").at("dir").get<std::string>());
			logDir.append("log");
			fs::create_directories(logDir);
		}

		// output: Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/output
		{
			fs::path outputDir(config_.at("data").at("dir").get<std::string>());
			outputDir.append("output");
			fs::create_directories(outputDir);
		}

		// plugin
		{
			// get plugin catalogue
			nlohmann::json catalogue;
			Plugin::getCatalogue(config_, catalogue);

			// initialize Doppelganger::Plugin instances
			for (const auto &pluginEntry : catalogue.items())
			{
				const std::string &name = pluginEntry.key();
				const nlohmann::json &pluginInfo = pluginEntry.value();
				plugin_[name] = std::make_unique<Doppelganger::Plugin>(name, pluginInfo);
			}

			// install plugins
			{
				const nlohmann::ordered_json installedPluginJson = config_.at("plugin").at("installed");
				std::vector<std::string> sortedInstalledPluginName(installedPluginJson.size());
				for (const auto &installedPlugin : installedPluginJson.items())
				{
					const std::string &name = installedPlugin.key();
					const nlohmann::json &value = installedPlugin.value();
					const int index = value.at("priority").get<int>();
					sortedInstalledPluginName.at(index) = name;
				}

				for (const auto &plugin : installedPluginJson.items())
				{
					const std::string &name = plugin.key();
					const nlohmann::json &info = plugin.value();
					const std::string &version = info.at("version").get<std::string>();

					if (plugin_.find(name) != plugin_.end() && version.size() > 0)
					{
						plugin_.at(name)->install(config_, version);
					}
					else
					{
						std::stringstream ss;
						ss << "Plugin \"" << name << "\" (" << version << ")"
						   << " is NOT found.";
						Util::log(ss.str(), "ERROR", config_);
					}
				}
			}

			// install non-optional plugins
			//   note: non-optional plugins are always installed to both Core and Room
			for (const auto &pluginEntry : catalogue.items())
			{
				const std::string &name = pluginEntry.key();
				const nlohmann::json &pluginInfo = pluginEntry.value();
				const bool &optional = pluginInfo.at("optional").get<bool>();
				if (!optional)
				{
					if (plugin_.find(name) != plugin_.end())
					{
						if (plugin_.at(name)->installedVersion_.size() == 0)
						{
							plugin_.at(name)->install(config_, std::string("latest"));
						}
					}
					else
					{
						std::stringstream ss;
						ss << "Non-optional plugin \"" << name << "\" (latest)"
						   << " is NOT found.";
						Util::log(ss.str(), "ERROR", config_);
					}
				}
			}
		}
	}

	void Room::joinWS(const WSSession &session)
	{
		std::lock_guard<std::mutex> lock(mutexWS_);
		std::visit([this](const auto &session_)
				   { websocketSessions_[session_->UUID_] = session_; },
				   session);
	}

	void Room::leaveWS(const std::string &sessionUUID)
	{
		std::lock_guard<std::mutex> lock(mutexWS_);
		websocketSessions_.erase(sessionUUID);
		// todo?
		// update cursors
	}

	void Room::broadcastWS(const std::string &APIName, const std::string &sourceUUID, const nlohmann::json &broadcast, const nlohmann::json &response)
	{
		std::lock_guard<std::mutex> lock(mutexWS_);
		nlohmann::json broadcastJson = nlohmann::json::object();
		nlohmann::json responseJson = nlohmann::json::object();
		if (!broadcast.empty())
		{
			broadcastJson["API"] = APIName;
			broadcastJson["parameters"] = broadcast;
		}
		if (!response.empty())
		{
			responseJson["API"] = APIName;
			responseJson["parameters"] = response;
		}
		const std::shared_ptr<const std::string> broadcastMessage = std::make_shared<const std::string>(broadcastJson.dump());
		const std::shared_ptr<const std::string> responseMessage = std::make_shared<const std::string>(responseJson.dump());

		for (const auto &uuid_session : websocketSessions_)
		{
			const std::string &sessionUUID = uuid_session.first;
			const WSSession &session = uuid_session.second;
			if (sessionUUID != sourceUUID)
			{
				if (!broadcast.empty())
				{
					std::visit([&broadcastMessage](const auto &session_)
							   { session_->send(broadcastMessage); },
							   session);
				}
			}
			else
			{
				if (!response.empty())
				{
					std::visit([&responseMessage](const auto &session_)
							   { session_->send(responseMessage); },
							   session);
				}
			}
		}
	}

	// void Room::storeHistory(const nlohmann::json &diff, const nlohmann::json &diffInv)
	// {
	// 	// erase future elements in the edit history
	// 	if (editHistory.index + 0 < editHistory.diffFromNext.size())
	// 	{
	// 		editHistory.diffFromNext.erase(editHistory.diffFromNext.begin() + (editHistory.index + 0), editHistory.diffFromNext.end());
	// 	}
	// 	if (editHistory.index + 1 < editHistory.diffFromPrev.size())
	// 	{
	// 		editHistory.diffFromPrev.erase(editHistory.diffFromPrev.begin() + (editHistory.index + 1), editHistory.diffFromPrev.end());
	// 	}
	// 	// store diff/diffInv
	// 	editHistory.diffFromNext.emplace_back(diffInv);
	// 	editHistory.diffFromPrev.emplace_back(diff);
	// 	editHistory.index = editHistory.diffFromNext.size();
	// }
}

#endif
