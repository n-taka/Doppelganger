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
	// response = [ // we use array to define the order of plugin install
	//  {
	//   "name": "pluginNameA",
	//   "modulePath": "path/to/module.js",
	//   "installedVersion": "latest",
	//  },
	//  {
	//   "name": "pluginNameB",
	//   "modulePath": "path/to/module.js",
	//   "installedVersion": "1.0.2",
	//  },
	//  ...
	// ]
	// broadcast = {
	// }

	// create response/broadcast
	response = nlohmann::json::array();
	broadcast = nlohmann::json::object();

	const std::unordered_map<std::string, std::shared_ptr<Doppelganger::Plugin>> &plugins = room->core->plugin;
	for (const auto &name_plugin : plugins)
	{
		const std::string &name = name_plugin.first;
		const std::shared_ptr<Doppelganger::Plugin> plugin = name_plugin.second;

		if (plugin->installedVersion.size() > 0)
		{
			nlohmann::json pluginInfo = nlohmann::json::object();
			pluginInfo["name"] = name;
			pluginInfo["modulePath"] = "todo";
			pluginInfo["installedVersion"] = plugin->installedVersion;

			response.push_back(pluginInfo);
		}
	}
}

#endif
