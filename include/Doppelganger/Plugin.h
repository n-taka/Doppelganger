#ifndef PLUGIN_H
#define PLUGIN_H

#if defined(_WIN64)
#ifdef DLL_EXPORT
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif
#elif defined(__APPLE__)
#define DECLSPEC
#elif defined(__linux__)
#define DECLSPEC
#endif

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
#include <variant>
#include <nlohmann/json.hpp>

namespace Doppelganger
{
	class Core;
	class Room;

	class DECLSPEC Plugin
	{
	public:
		Plugin(
			const std::string &name,
			const nlohmann::json &parameters,
			const fs::path &pluginsDir);
		~Plugin() {}

		const std::string name_;
		const nlohmann::json parameters_;
		const fs::path pluginsDir_;
		fs::path pluginDir;
		std::string installedVersion;
		bool hasModuleJS;

		void install(
			const std::variant<std::shared_ptr<Doppelganger::Core>, std::shared_ptr<Doppelganger::Room>> &coreRoom,
			const std::string &version);
		void pluginProcess(const std::shared_ptr<Doppelganger::Room> &room, const nlohmann::json &parameters, nlohmann::json &response, nlohmann::json &broadcast);
	};
} // namespace

#endif
