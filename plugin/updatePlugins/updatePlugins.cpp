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
	// parameters = [
	//  {
	//   "name": "pluginName-A",
	//   "version": "new version"
	//  },
	//  {
	//   "name": "pluginName-B",
	//   "version": "" (empty string indicates this plugin is not installed)
	//  },
	//  ...
	// ]

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

	// validate the parameters
	nlohmann::json installedArray = nlohmann::json::array();
	for (const auto &entry : parameters)
	{
		const std::string &name = entry.at("name").get<std::string>();
		const std::string &version = entry.at("version").get<std::string>();

		if (room->core->plugin.find(name) != room->core->plugin.end())
		{
			const std::shared_ptr<Doppelganger::Plugin> &plugin = room->core->plugin.at(name);
			if (version.size() > 0)
			{
				const nlohmann::json &pluginParameters = plugin->parameters;
				std::string versionToBeInstall = version;
				if (version == "latest")
				{
					versionToBeInstall = pluginParameters.at("latest").get<std::string>();
				}
				if (pluginParameters.at("versions").contains(versionToBeInstall))
				{
					installedArray.push_back(entry);
				}
				else
				{
					std::stringstream ss;
					ss << "Plugin \"";
					ss << name;
					ss << "\" invalid version \"";
					ss << version;
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

	fs::path installedPluginJsonPath(room->core->config.at("plugin").at("dir").get<std::string>());
	installedPluginJsonPath.append("installed.json");

	// write to file
	std::ofstream ofs(installedPluginJsonPath);
	ofs << installedArray.dump(4);
	ofs.close();
}

#endif
