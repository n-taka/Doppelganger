#ifndef PLUGIN_H
#define PLUGIN_H

#if defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#elif defined(__APPLE__)
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#elif defined(__linux__)
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <nlohmann/json.hpp>

#include "Doppelganger/Util/download.h"

namespace Doppelganger
{
	class Core;
	class Room;

	class Plugin
	{
	public:
		Plugin(){};

		void install(
			const std::weak_ptr<Room> &room,
			const std::string &version);
		void pluginProcess(
			const std::weak_ptr<Core> &core,
			const std::weak_ptr<Room> &room,
			const nlohmann::json &parameters,
			nlohmann::json &response,
			nlohmann::json &broadcast);
		void pluginProcess(
			const std::weak_ptr<Room> &room,
			const nlohmann::json &parameters,
			nlohmann::json &response,
			nlohmann::json &broadcast);

		struct InstalledVersionInfo
		{
			std::string name;
			std::string version;
		};

		struct VersionResourceInfo
		{
			std::string version;
			std::string URL;
		};

	public:
		////
		// parameters stored in nlohmann::json
		std::string name_;
		std::unordered_map<std::string, std::string> description_;
		bool optional_;
		std::string UIPosition_;
		bool hasModuleJS_;
		std::vector<VersionResourceInfo> versions_;
		std::string installedVersion_;
		fs::path dir_;

		////
		// parameters **NOT** stored in nlohmann::json
		// note: all parameters are stored in nlohmann::json
	};

	////
	// nlohmann::json conversion
	////
	void to_json(nlohmann::json &json, const Plugin &plugin);
	void from_json(const nlohmann::json &json, Plugin &plugin);
}

////
// json format for Doppelganger::Plugin
////
// {
//     "name": "sortMeshes",
//     "description": {
//         "en": "description in English",
//         "ja": "description in Japanese"
//     },
//     "optional": false,
//     "UIPosition": "topRight",
//     "versions": [
//         {
//             "version": "2.0.0",
//             "URL": "https://n-taka.info/nextcloud/s/jZzAeW7eMsmD3Yr/download/sortMeshes.zip"
//         }
//     ],
//     "installedVersion": "latest",
//     "dir": "path/to/plugin_itself"
// }

#endif
