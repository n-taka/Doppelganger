#ifndef ROOM_CPP
#define ROOM_CPP

#include "Doppelganger/Room.h"

#include <fstream>
#include <sstream>

#include "Doppelganger/Plugin.h"
#include "Doppelganger/WebsocketSession.h"
#include "Doppelganger/Util/getCurrentTimestampAsString.h"
#include "Doppelganger/Util/getPluginCatalogue.h"
#include "Doppelganger/Util/log.h"

namespace Doppelganger
{
	Room::Room()
	{
	}

	void Room::setup(
		const std::string &UUID,
		const nlohmann::json &configCore)
	{
		// inherit from Core
		config = configCore;
		// remove config for core
		config.erase("dataDir");
		// add room-specific contents
		config["UUID"] = UUID;
		config["meshes"] = nlohmann::json::object();
		config.at("plugin").at("reInstall") = true;
		config["history"] = nlohmann::json::object();
		config.at("history")["index"] = 0;
		config.at("history")["diffFromPrev"] = nlohmann::json::array();
		config.at("history").at("diffFromPrev").push_back(nlohmann::json::object());
		config.at("history")["diffFromNext"] = nlohmann::json::array();
		config.at("history")["extension"] = nlohmann::json::object();
		applyCurrentConfig();

		// log
		{
			std::stringstream ss;
			ss << "New room \"" << config.at("UUID").get<std::string>() << "\" is created.";
			// we log this message to Core
			Util::log(ss.str(), "SYSTEM", configCore);
		}
	}

	void Room::applyCurrentConfig()
	{
		// dataDir: Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/
		fs::path dataDir_(config.at("DoppelgangerRootDir").get<std::string>());
		if (!config.contains("dataDir"))
		{
			dataDir_.append("data");
			std::string dirName("");
			dirName += Util::getCurrentTimestampAsString(false);
			dirName += "-";
			dirName += config.at("UUID").get<std::string>();
			dataDir_.append(dirName);
			config["dataDir"] = dataDir_.string();
			fs::create_directories(dataDir_);
		}

		// log: Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/log
		{
			fs::path logDir(config.at("dataDir").get<std::string>());
			logDir.append("log");
			fs::create_directories(logDir);
		}

		// output: Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/output
		{
			fs::path outputDir(config.at("dataDir").get<std::string>());
			outputDir.append("output");
			fs::create_directories(outputDir);
		}

		// plugin: Doppelganger/data/YYYYMMDDTHHMMSS-room-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/plugin
		{
			fs::path pluginDir(config.at("dataDir").get<std::string>());
			pluginDir.append("plugin");
			fs::create_directories(pluginDir);
		}
		if (config.contains("plugin"))
		{
			if (config.at("plugin").contains("reInstall") && config.at("plugin").at("reInstall").get<bool>())
			{
				config.at("plugin").at("reInstall") = false;
				if (config.at("plugin").contains("listURL"))
				{
					// get plugin catalogue
					fs::path pluginDir(config.at("DoppelgangerRootDir").get<std::string>());
					pluginDir.append("plugin");
					nlohmann::json catalogue;
					Util::getPluginCatalogue(pluginDir, config.at("plugin").at("listURL"), catalogue);

					// update plugins
					{
						// remove old plugins
						plugin_.clear();

						// initialize Doppelganger::Plugin instances
						for (const auto &pluginEntry : catalogue)
						{
							const Doppelganger::Plugin plugin = pluginEntry.get<Doppelganger::Plugin>();
							plugin_[plugin.name_] = plugin;
						}

						// install plugins
						{
							for (const auto &installedPluginJson : config.at("plugin").at("installed"))
							{
								const std::string name = installedPluginJson.at("name").get<std::string>();
								const std::string version = installedPluginJson.at("version").get<std::string>();

								if (plugin_.find(name) != plugin_.end() && version.length() > 0)
								{
#if defined(_WIN64)
									plugin_.at(name).install(weak_from_this(), version);
#elif defined(__APPLE__)
									plugin_.at(name).install(std::weak_ptr<Room>(shared_from_this()), version);
#elif defined(__linux__)
									plugin_.at(name).install(weak_from_this(), version);
#endif
								}
								else
								{
									std::stringstream ss;
									ss << "Plugin \"" << name << "\" (" << version << ")"
									   << " is NOT found in the catalogue.";
									Util::log(ss.str(), "ERROR", config);
								}
							}
						}

						// install non-optional plugins
						for (auto &name_plugin : plugin_)
						{
							const std::string &name = name_plugin.first;
							Plugin &plugin = name_plugin.second;
							if (!plugin.optional_ && (plugin.installedVersion_.size() == 0))
							{
#if defined(_WIN64)
								plugin.install(weak_from_this(), std::string("latest"));
#elif defined(__APPLE__)
								plugin.install(std::weak_ptr<Room>(shared_from_this()), std::string("latest"));
#elif defined(__linux__)
								plugin.install(weak_from_this(), std::string("latest"));
#endif
								nlohmann::json installedPluginJson = nlohmann::json::object();
								installedPluginJson["name"] = name;
								installedPluginJson["version"] = std::string("latest");
								config.at("plugin").at("installed").push_back(installedPluginJson);
							}
						}
					}
				}
			}
		}

		// config.at("mesh") is maintained by using nlohmann::json::merge_patch
		// config.at("history") is maintained by using nlohmann::json::merge_patch
		// config.at("extension") is maintained by using nlohmann::json::merge_patch

		// "active" and "forceReload" are very critical and we take care of them in the last
		if (config.contains("active") && !config.at("active").get<bool>())
		{
			// shutdown...
			// TODO: close websocket sessions, etc...
			return;
		}

		if (config.contains("forceReload") && config.at("forceReload").get<bool>())
		{
			config.at("forceReload") = false;
			broadcastWS(std::string("forceReload"), std::string(""), nlohmann::json::object(), nlohmann::json(nullptr));
		}
	}

	void Room::joinWS(const WSSession &session)
	{
		std::lock_guard<std::mutex> lock(mutexWS_);
#if defined(_WIN64)
		std::visit(
#elif defined(__APPLE__)
		boost::apply_visitor(
#elif defined(__linux__)
		std::visit(
#endif
			[this](const auto &session_)
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
		if (!broadcast.is_null())
		{
			broadcastJson["API"] = APIName;
			broadcastJson["parameters"] = broadcast;
		}
		if (!response.is_null())
		{
			responseJson["API"] = APIName;
			responseJson["parameters"] = response;
		}
		const std::shared_ptr<const std::string> broadcastMessage = std::make_shared<const std::string>(broadcastJson.dump(-1, ' ', true));
		const std::shared_ptr<const std::string> responseMessage = std::make_shared<const std::string>(responseJson.dump(-1, ' ', true));

		for (const auto &uuid_session : websocketSessions_)
		{
			const std::string &sessionUUID = uuid_session.first;
			const WSSession &session = uuid_session.second;
			if (sessionUUID != sourceUUID)
			{
				if (!broadcast.is_null())
				{
#if defined(_WIN64)
					std::visit(
#elif defined(__APPLE__)
					boost::apply_visitor(
#elif defined(__linux__)
					std::visit(
#endif
						[&broadcastMessage](const auto &session_)
						{ session_->send(broadcastMessage); },
						session);
				}
			}
			else
			{
				if (!response.is_null())
				{
#if defined(_WIN64)
					std::visit(
#elif defined(__APPLE__)
					boost::apply_visitor(
#elif defined(__linux__)
					std::visit(
#endif
						[&responseMessage](const auto &session_)
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
