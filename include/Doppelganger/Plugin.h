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
#include <nlohmann/json.hpp>

namespace Doppelganger
{
	class Plugin
	{
	public:
		Plugin(
			const std::string &name,
			const nlohmann::json &parameters);

		void install(
			nlohmann::json &config,
			const std::string &version);
		void pluginProcess(
			nlohmann::json &configCore,
			nlohmann::json &configRoom,
			nlohmann::json &configMesh,
			const nlohmann::json &parameters,
			nlohmann::json &response,
			nlohmann::json &broadcast);

		static void getCatalogue(
			const nlohmann::json &config,
			nlohmann::json &catalogue);

	public:
		////
		// parameter for this plugin
		const std::string name_;
		const nlohmann::json parameters_;
		fs::path pluginDir_;
		std::string installedVersion_;
		bool hasModuleJS_;
	};
}

#endif
