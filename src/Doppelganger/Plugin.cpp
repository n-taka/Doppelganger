#ifndef PLUGIN_CPP
#define PLUGIN_CPP

#include "Doppelganger/Plugin.h"
#include "Doppelganger/Core.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/Util/download.h"
#include "Doppelganger/Util/unzip.h"
#include <string>
#include <fstream>
#include <streambuf>
#if defined(_WIN64)
#include "windows.h"
#include "libloaderapi.h"
#elif defined(__APPLE__)
#include <dlfcn.h>
#elif defined(__linux__)
#include <dlfcn.h>
#endif

namespace
{
	////
	// typedef for loading API from dll/lib
#if defined(_WIN64)
	typedef void(__stdcall *APIPtr_t)(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &, nlohmann::json &, nlohmann::json &);
#elif defined(__APPLE__)
	typedef void (*APIPtr_t)(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &, nlohmann::json &, nlohmann::json &);
#elif defined(__linux__)
	typedef void (*APIPtr_t)(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &, nlohmann::json &, nlohmann::json &);
#endif

	bool functionCall(
		const fs::path &dllPath,
		const std::string &functionName,
		const std::shared_ptr<Doppelganger::Room> &room,
		const nlohmann::json &parameters,
		nlohmann::json &response,
		nlohmann::json &broadcast)
	{
#if defined(_WIN64)
		HINSTANCE handle = LoadLibrary(dllPath.string().c_str());
#elif defined(__APPLE__)
		void *handle;
		handle = dlopen(dllPath.string().c_str(), RTLD_LAZY);
#elif defined(__linux__)
		void *handle;
		handle = dlopen(dllPath.string().c_str(), RTLD_LAZY);
#endif

		if (handle != NULL)
		{
#if defined(_WIN64)
			FARPROC lpfnDllFunc = GetProcAddress(handle, functionName.c_str());
#elif defined(__APPLE__)
			void *lpfnDllFunc = dlsym(handle, functionName.c_str());
#elif defined(__linux__)
			void *lpfnDllFunc = dlsym(handle, functionName.c_str());
#endif
			if (!lpfnDllFunc)
			{
				// error
#if defined(_WIN64)
				FreeLibrary(handle);
#elif defined(__APPLE__)
				dlclose(handle);
#elif defined(__linux__)
				dlclose(handle);
#endif
				return false;
			}
			else
			{
				reinterpret_cast<APIPtr_t>(lpfnDllFunc)(room, parameters, response, broadcast);
#if defined(_WIN64)
				FreeLibrary(handle);
#elif defined(__APPLE__)
				dlclose(handle);
#elif defined(__linux__)
				dlclose(handle);
#endif
				return true;
			}
		}
		else
		{
			return false;
		}
	};
}

namespace Doppelganger
{
	Plugin::Plugin(
		const std::string &name,
		const nlohmann::json &parameters,
		const fs::path &pluginsDir)
		: name_(name), parameters_(parameters), pluginsDir_(pluginsDir), hasModuleJS(false)
	{
	}

	void Plugin::install(
		const std::variant<std::shared_ptr<Doppelganger::Core>, std::shared_ptr<Doppelganger::Room>> &coreRoom,
		const std::string &version)
	{
		const std::string actualVersion((version == "latest") ? parameters_.at("versions").at(0).at("version").get<std::string>() : version);

		pluginDir = pluginsDir_;
		std::string dirName("");
		dirName += name_;
		dirName += "_";
		dirName += actualVersion;
		pluginDir.append(dirName);

		if (!fs::exists(pluginDir))
		{
			bool versionFound = false;
			for (const auto &versionEntry : parameters_.at("versions"))
			{
				const std::string &pluginVersion = versionEntry.at("version").get<std::string>();
				if (pluginVersion == actualVersion)
				{
					const std::string &pluginURL = versionEntry.at("URL").get<std::string>();
					fs::path zipPath(pluginsDir_);
					zipPath.append("tmp.zip");
					if (Util::download(pluginURL, zipPath))
					{
						Util::unzip(zipPath, pluginDir);
						// erase temporary file
						fs::remove_all(zipPath);
						versionFound = true;
						break;
					}
					else
					{
						// failure
						std::stringstream ss;
						ss << "Plugin \"" << name_ << "\" (";
						if (version == "latest")
						{
							ss << "latest, ";
						}
						ss << actualVersion << ")"
						   << " is NOT loaded correctly. (Download)";
						std::visit([&ss](const auto &v)
								   { v->logger.log(ss.str(), "ERROR"); },
								   coreRoom);
						return;
					}
				}
			}

			if (!versionFound)
			{
				// failure
				std::stringstream ss;
				ss << "Plugin \"" << name_ << "\" (";
				if (version == "latest")
				{
					ss << "latest, ";
				}
				ss << actualVersion << ")"
				   << " is NOT loaded correctly. (No such version)";
				std::visit([&ss](const auto &v)
						   { v->logger.log(ss.str(), "ERROR"); },
						   coreRoom);
				return;
			}
			else
			{
				std::stringstream ss;
				ss << "Plugin \"" << name_ << "\" (";
				if (version == "latest")
				{
					ss << "latest, ";
				}
				ss << actualVersion << ")"
				   << " is loaded.";
				std::visit([&ss](const auto &v)
						   { v->logger.log(ss.str(), "SYSTEM"); },
						   coreRoom);
			}
		}
		else
		{
			// already downloaded
			std::stringstream ss;
			ss << "Plugin \"" << name_ << "\" (";
			if (version == "latest")
			{
				ss << "latest, ";
			}
			ss << actualVersion << ")"
			   << " is already downloaded. We reuse it.";
			std::visit([&ss](const auto &v)
					   { v->logger.log(ss.str(), "SYSTEM"); },
					   coreRoom);
		}
		installedVersion = version;

		// javascript module (if exists)
		{
			const std::string moduleName("module.js");
			fs::path modulePath(pluginDir);
			modulePath.append(moduleName);
			hasModuleJS = fs::exists(modulePath);
		}

		// update installed plugin list "installed.json"
		{
			fs::path installedPluginJsonPath;
			if (std::holds_alternative<std::shared_ptr<Doppelganger::Core>>(coreRoom))
			{
				const std::shared_ptr<Doppelganger::Core> &core = std::get<std::shared_ptr<Doppelganger::Core>>(coreRoom);
				installedPluginJsonPath = core->DoppelgangerRootDir;
				installedPluginJsonPath.append("plugin");
			}
			else // std::shared_ptr<Doppelganger::Room>
			{
				const std::shared_ptr<Doppelganger::Room> &room = std::get<std::shared_ptr<Doppelganger::Room>>(coreRoom);
				installedPluginJsonPath = room->dataDir;
			}
			installedPluginJsonPath.append("installed.json");

			nlohmann::json installedPluginJson;

			std::ifstream ifs(installedPluginJsonPath.string());
			installedPluginJson = nlohmann::json::parse(ifs);
			ifs.close();

			nlohmann::json pluginInfo = nlohmann::json::object();
			pluginInfo["name"] = name_;
			pluginInfo["version"] = installedVersion;
			installedPluginJson.push_back(pluginInfo);

			std::ofstream ofs(installedPluginJsonPath.string());
			ofs << installedPluginJson.dump(4);
			ofs.close();
		}
	}

	void Plugin::pluginProcess(const std::shared_ptr<Doppelganger::Room> &room, const nlohmann::json &parameters, nlohmann::json &response, nlohmann::json &broadcast)
	{
		fs::path dllPath(pluginDir);
		std::string dllName(name_);
#if defined(_WIN64)
		dllPath.append("Windows");
		dllName += ".dll";
#elif defined(__APPLE__)
		dllPath.append("Darwin");
		dllName = "lib" + dllName + ".so";
#elif defined(__linux__)
		dllPath.append("Linux");
		dllName = "lib" + dllName + ".so";
#endif
		dllPath.append(dllName);
		// c++ functions (.dll/.so) (if exists)
		if (fs::exists(dllPath))
		{
			if (!functionCall(dllPath, "pluginProcess", room, parameters, response, broadcast))
			{
				std::stringstream ss;
				ss << "Plugin \"" << name_ << "\" (" << installedVersion << ")"
				   << " is NOT called correctly.";
				room->logger.log(ss.str(), "ERROR");
			}
		}
	}
} // namespace

#endif
