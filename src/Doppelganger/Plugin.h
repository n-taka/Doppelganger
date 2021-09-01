#ifndef PLUGIN_H
#define PLUGIN_H

#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#if defined(_WIN32) || defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#endif

namespace Doppelganger
{
	class Core;
	class Room;

	class Plugin
	{
	public:
		Plugin(const std::shared_ptr<Core> &core_);
		~Plugin() {}
		const std::shared_ptr<Core> core;

		void loadPlugin(const std::string &name, const std::string &pluginUrl);
		void loadPlugin(const fs::path &pluginDir);
		// following variables are iniaitlized by calling loadPlugin
		std::string name;
		std::string author;
		std::string version;
		int priority;
		////
		// typedef for API
		//   all API have this signature, and other parameters (e.g. meshUUID) are supplied within the parameter json.
		typedef std::function<void(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &, nlohmann::json &)> API_t;
		API_t func;
		std::string moduleJS;
	};
} // namespace

#endif
