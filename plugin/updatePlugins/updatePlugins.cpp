#ifndef UPDATEPLUGINS_CPP
#define UPDATEPLUGINS_CPP

#include "../pluginHeader.h"
#include "nlohmann/json.hpp"
#include "Doppelganger/Core.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/Plugin.h"
#include "Doppelganger/Logger.h"

#include <string>
#include <fstream>

extern "C" DLLEXPORT void metadata(const std::shared_ptr<Doppelganger::Room> &room, const nlohmann::json &parameters, nlohmann::json &response, nlohmann::json &broadcast)
{
	response["author"] = "Kazutaka Nakashima";
	response["version"] = "1.0.0";
	broadcast = nlohmann::json::object();
}

extern "C" DLLEXPORT void pluginProcess(const std::shared_ptr<Doppelganger::Room> &room, const nlohmann::json &parameters, nlohmann::json &response, nlohmann::json &broadcast)
{
	////
	// [IN]
	// parameters = {
	//  "pluginNameA": "new version",
	//  "pluginNameB": "new version",
	//  ...
	// }

	// [OUT]
	// response = {
	// }
	// broadcast = {
	// }

	// create response/broadcast
	response = nlohmann::json::object();
	broadcast = nlohmann::json::object();

	// this plugin updates installed.json
	// actual update is performed on the next boot

	fs::path installedPluginJsonPath(room->core->config.at("plugin").at("dir").get<std::string>());
	installedPluginJsonPath.append("installed.json");
	if (fs::exists(installedPluginJsonPath))
	{
		// read current installed.json
		std::ifstream ifs(installedPluginJsonPath);
		nlohmann::json installedPluginJson = nlohmann::json::parse(ifs);
		ifs.close();

		const std::unordered_map<std::string, std::shared_ptr<Doppelganger::Plugin>> &plugins = room->core->plugin;
		for (const auto &pluginToBeUpdated : parameters.items())
		{
			const std::string &name = pluginToBeUpdated.key();
			std::string newVersion = pluginToBeUpdated.value().get<std::string>();
			if (plugins.find(name) != plugins.end())
			{
				installedPluginJson.erase(name);
				if (newVersion != "Uninstall")
				{
					const std::shared_ptr<Doppelganger::Plugin> &pluginPtr = plugins.at(name);
					const nlohmann::json &pluginParameters = pluginPtr->parameters;
					if (newVersion == "latest")
					{
						newVersion = pluginParameters.at("latest").get<std::string>();
					}
					if (pluginParameters.at("versions").contains(newVersion))
					{
						nlohmann::json pluginJson = nlohmann::json::object();
						pluginJson["version"] = newVersion;
						installedPluginJson[name] = pluginJson;
					}
					else
					{
						std::stringstream ss;
						ss << "Plugin \"";
						ss << name;
						ss << "\" invalid version \"";
						ss << newVersion;
						ss << "\" is specified.";
						room->core->logger.log(ss.str(), "ERROR");
					}
				}
			}
			else
			{
				std::stringstream ss;
				ss << "Unknown plugin \"";
				ss << name;
				ss << "\" is specified.";
				room->core->logger.log(ss.str(), "ERROR");
			}
		}
		// write to file
		std::ofstream ofs(installedPluginJsonPath);
		ofs << installedPluginJson.dump(4);
		ofs.close();
	}
	else
	{
		std::stringstream ss;
		ss << "\"installed.json\" is not found.";
		room->core->logger.log(ss.str(), "ERROR");
	}
}

#endif
