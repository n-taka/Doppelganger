#ifndef LISTPLUGINS_CPP
#define LISTPLUGINS_CPP

#include "../pluginHeader.h"
#include "nlohmann/json.hpp"
#include "Doppelganger/Room.h"
#include "Doppelganger/Core.h"

#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#if defined(_WIN32) || defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#endif

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
	//  "server": {
	//      "protocol": "http://",
	//      "host": "127.0.0.1",
	//      "port": 0
	//  },
	//  "log": {
	//      "dir": "",
	//      "level": [
	//          "SYSTEM",
	//          "APICALL",
	//          "WSCALL",
	//          "ERROR",
	//          "MISC",
	//          "DEBUG"
	//      ],
	//      "type": [
	//          "STDOUT",
	//          "FILE"
	//      ]
	//  },
	//  "output": {
	//      "type": "local",
	//      "dir": ""
	//  },
	//  "plugin": {
	//      "dir": "",
	//      "listURL": [
	//          "https://n-taka.info/nextcloud/s/KKH2dWmTRT5wRA5/download/defaultPluginList.json"
	//      ]
	//  },
	//  "browser": {
	//      "type": "chrome",
	//      "path": "",
	//      "openAs": "app",
	//      "openOnBoot": true
	//  }
	// }

	// [OUT]
	// response = {
	//  JSON config after update
	// }
	// broadcast = {
	// }

	// create response/broadcast
	response = nlohmann::json::object();
	broadcast = nlohmann::json::object();

	response = room->core->configFileContent;

	// current config always has 2-depth, we can easily implement as follows;
	for (const auto &parameterItem : parameters.items())
	{
		// add parameter if is doesn't exist
		if (!response.contains(parameterItem.key()))
		{
			response[parameterItem.key()] = nlohmann::json::object();
		}

		const nlohmann::json &parametersJson = parameters.at(parameterItem.key());
		nlohmann::json &responseJson = response.at(parameterItem.key());
		for (const auto &item : parametersJson.items())
		{
			responseJson[item.key()] = item.value();
		}
	}

	// write to config.json
	//   for human readability, we use .dump(4);
	{
		std::lock_guard<std::mutex> lock(room->core->systemParams.mutex);
		fs::path configPath(room->core->systemParams.workingDir);
		configPath.append("config.json");
		std::ofstream ofs(configPath);
		ofs << response.dump(4);
		ofs.close();
	}
}

#endif
