#ifndef PLUGIN_H
#define PLUGIN_H

#if defined(_WIN32) || defined(_WIN64)
#ifdef DLL_EXPORT
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif
#else
#define DECLSPEC
#endif

#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#if defined(__APPLE__)
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

namespace Doppelganger
{
	class Core;
	class Room;

	class DECLSPEC Plugin
	{
	public:
		Plugin(const std::shared_ptr<Core> &core_, const std::string &name_, const nlohmann::json &parameters_);
		~Plugin() {}
		const std::shared_ptr<Core> core;
		const std::string name;
		const nlohmann::json parameters;

		void install(const std::string &version);
		std::string installedVersion;

		////
		// typedef for API
		//   all API have this signature, and other parameters (e.g. meshUUID) are supplied within the parameter json.
		typedef std::function<void(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &, nlohmann::json &, nlohmann::json &)> API_t;
		API_t func;
		bool hasModuleJS;

	private:
		void installFromDirectory(const fs::path &pluginDir);
	};
} // namespace

#endif
