#ifndef LOADPOLYGONMESH_CPP
#define LOADPOLYGONMESH_CPP

#include "../pluginHeader.h"
#include "nlohmann/json.hpp"
#include "Doppelganger/Core.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/Plugin.h"

#include <string>

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
	// }

	// [OUT]
	// response = {
	//  "pluginNameA": {
	//   "installedVersion": "" (if text is zero-length, which means not installed),
	//   "versions": ["latest", "1.0.2", "1.0.1", "1.0.0"],
	//   "description": "description text for this plugin",
	//   "optional": true|false
	//  },
	//  "pluginNameB": {
	//   "installedVersion": "" (if text is zero-length, which means not installed),
	//   "versions": ["latest", "1.0.2", "1.0.1", "1.0.0"],
	//   "description": "description text for this plugin",
	//   "optional": true|false
	//  },
	//  ...
	// }
	// broadcast = {
	// }

	// create response/broadcast
	response = nlohmann::json::object();
	broadcast = nlohmann::json::object();

	const std::unordered_map<std::string, std::shared_ptr<Doppelganger::Plugin>> &plugins = room->core->plugin;
	for (const auto &name_plugin : plugins)
	{
		const std::string &name = name_plugin.first;
		const std::shared_ptr<Doppelganger::Plugin> plugin = name_plugin.second;

		nlohmann::json pluginInfo = nlohmann::json::object();
		pluginInfo["installedVersion"] = plugin->installedVersion;
		nlohmann::json versionList = nlohmann::json::array();
		versionList.push_back(std::string("latest"));
		for (const auto &version_url : plugin->parameters.at("versions").items())
		{
			versionList.push_back(version_url.key());
		}
		pluginInfo["versions"] = versionList;
		pluginInfo["latest"] = plugin->parameters.at("latest");
		pluginInfo["description"] = plugin->parameters.at("description");
		pluginInfo["optional"] = plugin->parameters.at("optional");
		pluginInfo["hasModuleJS"] = plugin->hasModuleJS;

		response[name] = pluginInfo;
	}
}

#endif
