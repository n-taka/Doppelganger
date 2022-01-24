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
		// inherit from Core
		from_json(configCore);

		// set room specific contents
		UUID_ = UUID;
		history_.index = 0;
		history_.diffFromPrev.push_back(nlohmann::json::object());

		// apply current config
		applyCurrentConfig();

		// log
		{
			std::stringstream ss;
			ss << "New room \"" << UUID_ << "\" is created.";
			Util::log(ss.str(), "SYSTEM", dataDir_, logConfig_.level, logConfig_.type);
		}
	}

	void Room::to_json(nlohmann::json &json) const
	{
		json = nlohmann::json::object();
		json["active"] = active_;
		json["UUID"] = UUID_;
		json["DoppelgangerRootDir"] = DoppelgangerRootDir_.string();
		json["log"] = nlohmann::json::object();
		{
			json.at("log")["level"] = nlohmann::json::object();
			for (const auto &level_value : logConfig_.level)
			{
				const std::string &level = level_value.first;
				const bool &value = level_value.second;
				json.at("log").at("level")[level] = value;
			}
			json.at("log")["type"] = nlohmann::json::object();
			for (const auto &type_value : logConfig_.type)
			{
				const std::string &type = type_value.first;
				const bool &value = type_value.second;
				json.at("log").at("type")[type] = value;
			}
		}
		json["output"] = nlohmann::json::object();
		{
			json.at("output")["type"] = outputType_;
		}
		json["plugin"] = nlohmann::json::object();
		{
			json.at("plugin")["installed"] = nlohmann::json::array();
			for (const auto &plugin : installedPlugin_)
			{
				nlohmann::json pluginJson = nlohmann::json::object();
				pluginJson["name"] = plugin.name;
				pluginJson["version"] = plugin.version;
				json.at("plugin").at("installed").push_back(pluginJson);
			}
			json.at("plugin")["listURL"] = nlohmann::json::array();
			for (const auto &listURL : pluginListURL_)
			{
				json.at("plugin").at("listURL").push_back(listURL);
			}
		}
		json["meshes"] = nlohmann::json::object();
		for (const auto &uuid_mesh : meshes_)
		{
			const std::string &uuid = uuid_mesh.first;
			const TriangleMesh &mesh = uuid_mesh.second;
			json.at("meshes")[uuid] = mesh;
		}
		json["history"] = nlohmann::json::object();
		{
			json.at("history")["index"] = history_.index;
			json.at("history")["diffFromPrev"] = nlohmann::json::array();
			for (const auto &diff : history_.diffFromPrev)
			{
				json.at("history").at("diffFromPrev").push_back(diff);
			}
			json.at("history")["diffFromNext"] = nlohmann::json::array();
			for (const auto &diff : history_.diffFromNext)
			{
				json.at("history").at("diffFromNext").push_back(diff);
			}
		}
		json["extension"] = extension_;
	}

	void Room::from_json(const nlohmann::json &json)
	{
		// from_json is also used for initializing with configCore
		// so, we need to explicitly check with .contains(...)
		if (json.contains("active"))
		{
			active_ = json.at("active").get<bool>();
		}
		if (json.contains("UUID"))
		{
			UUID_ = json.at("UUID").get<std::string>();
		}
		if (json.contains("DoppelgangerRootDir"))
		{
			DoppelgangerRootDir_ = fs::path(json.at("DoppelgangerRootDir").get<std::string>());
		}
		if (json.contains("log"))
		{
			logConfig_.level.clear();
			for (const auto &level_value : json.at("log").at("level").items())
			{
				const std::string &level = level_value.key();
				const bool &value = level_value.value().get<bool>();
				logConfig_.level[level] = value;
			}
			logConfig_.type.clear();
			for (const auto &type_value : json.at("log").at("type").items())
			{
				const std::string &type = type_value.key();
				const bool &value = type_value.value().get<bool>();
				logConfig_.type[type] = value;
			}
		}

		if (json.contains("output"))
		{
			outputType_ = json.at("output").at("type").get<std::string>();
		}

		if (json.contains("plugin"))
		{
			installedPlugin_.clear();
			for (const auto &plugin : json.at("plugin").at("installed"))
			{
				PluginInfo pluginInfo;
				pluginInfo.name = plugin.at("name").get<std::string>();
				pluginInfo.version = plugin.at("version").get<std::string>();
				installedPlugin_.push_back(pluginInfo);
			}

			pluginListURL_.clear();
			for (const auto &listURL : json.at("plugin").at("listURL"))
			{
				pluginListURL_.push_back(listURL.get<std::string>());
			}
		}

		if (json.contains("meshes"))
		{
			meshes_.clear();
			for (const auto &uuid_mesh : json.at("meshes").items())
			{
				const std::string &uuid = uuid_mesh.key();
				const TriangleMesh mesh = uuid_mesh.value().get<TriangleMesh>();
				meshes_[uuid] = mesh;
			}
		}

		if (json.contains("history"))
		{
			history_.index = json.at("history").at("index").get<int>();

			history_.diffFromPrev.clear();
			for (const auto &diff : json.at("history").at("diffFromPrev"))
			{
				history_.diffFromPrev.push_back(diff);
			}

			history_.diffFromNext.clear();
			for (const auto &diff : json.at("history").at("diffFromNext"))
			{
				history_.diffFromNext.push_back(diff);
			}
		}

		if (json.contains("extension"))
		{
			extension_ = json.at("extension");
		}
	}

	void Room::applyCurrentConfig()
	{
		if (!active_)
		{
			// shutdown...
			// TODO: close websocket sessions, etc...
			return;
		}

		// dataDir: Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/
		if (!fs::exists(dataDir_))
		{
			dataDir_ = DoppelgangerRootDir_;
			dataDir_.append("data");
			std::string dirName("");
			dirName += Util::getCurrentTimestampAsString(false);
			dirName += "-";
			dirName += UUID_;
			dataDir_.append(dirName);
			fs::create_directories(dataDir_);
		}

		// log: Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/log
		{
			fs::path logDir(dataDir_);
			logDir.append("log");
			fs::create_directories(logDir);
		}

		// output: Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/output
		{
			fs::path outputDir(dataDir_);
			outputDir.append("output");
			fs::create_directories(outputDir);
		}

		// plugin
		{
			// remove outdated ones
			plugin_.clear();

			// get plugin catalogue
			fs::path pluginDir(DoppelgangerRootDir_);
			pluginDir.append("plugin");
			nlohmann::json catalogue;
			Plugin::getCatalogue(pluginDir, pluginListURL_, catalogue);

			// initialize Doppelganger::Plugin instances
			for (const auto &pluginEntry : catalogue)
			{
				Doppelganger::Plugin plugin = pluginEntry.get<Doppelganger::Plugin>();
				plugin_[plugin.name_] = plugin;
			}

			// install plugins
			{
				for (const auto &plugin : installedPlugin_)
				{
					const std::string &name = plugin.name;
					const std::string &version = plugin.version;

					if (plugin_.find(name) != plugin_.end() && version.size() > 0)
					{
						plugin_.at(name).install(weak_from_this(), version);
					}
					else
					{
						std::stringstream ss;
						ss << "Plugin \"" << name << "\" (" << version << ")"
						   << " is NOT found.";
						Util::log(ss.str(), "ERROR", dataDir_, logConfig_.level, logConfig_.type);
					}
				}
			}

			// install non-optional plugins
			for (auto &name_plugin : plugin_)
			{
				Plugin &plugin = name_plugin.second;
				if (!plugin.optional_ && (plugin.installedVersion_.size() == 0))
				{
					plugin.install(weak_from_this(), std::string("latest"));
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

	
		// add Room specific parameters
		// to plugin...?
		// interface
		// camera
		// {
		// 	config_["camera"] = nlohmann::json::object();
		// 	config_["camera"]["target"] = nlohmann::json::object();
		// 	config_["camera"]["target"]["x"] = 0.0;
		// 	config_["camera"]["target"]["y"] = 0.0;
		// 	config_["camera"]["target"]["z"] = 0.0;
		// 	config_["camera"]["target"]["timestamp"] = 0;
		// 	config_["camera"]["position"] = nlohmann::json::object();
		// 	config_["camera"]["position"]["x"] = -30.0;
		// 	config_["camera"]["position"]["y"] = 40.0;
		// 	config_["camera"]["position"]["z"] = 30.0;
		// 	config_["camera"]["position"]["timestamp"] = 0;
		// 	config_["camera"]["up"] = nlohmann::json::object();
		// 	config_["camera"]["up"]["x"] = 0.0;
		// 	config_["camera"]["up"]["y"] = 1.0;
		// 	config_["camera"]["up"]["z"] = 0.0;
		// 	config_["camera"]["up"]["timestamp"] = 0;
		// 	config_["camera"]["zoom"] = nlohmann::json::object();
		// 	config_["camera"]["zoom"]["value"] = 1.0;
		// 	config_["camera"]["zoom"]["timestamp"] = 0;
		// }
}

#endif
