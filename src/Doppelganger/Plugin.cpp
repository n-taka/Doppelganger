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
		const std::shared_ptr<Doppelganger::Core> &core,
		const std::string &name,
		const nlohmann::json &parameters)
		: core_(core), name_(name), parameters_(parameters)
	{
	}

	void Plugin::install(
		const std::shared_ptr<Doppelganger::Room> &room,
		const std::string &version,
		const bool persistent)
	{
		const std::string actualVersion((version == "latest") ? parameters_.at("versions").at(0).at("version").get<std::string>() : version);

		pluginDir = core_->DoppelgangerRootDir;
		pluginDir.append("plugin");
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
					fs::path zipPath(core_->DoppelgangerRootDir);
					zipPath.append("plugin");
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
						room->logger.log(ss.str(), "ERROR");
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
				room->logger.log(ss.str(), "ERROR");
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
				room->logger.log(ss.str(), "SYSTEM");
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
			room->logger.log(ss.str(), "SYSTEM");
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
			std::lock_guard<std::mutex> lock(room->mutexConfig);
			nlohmann::json config;
			room->getCurrentConfig(config);
			config.at("plugin").at("installed")[name_] = nlohmann::json::object();
			config.at("plugin").at("installed")[name_]["version"] = installedVersion;
			room->updateConfig(config);
		}
		if (persistent)
		{
			std::lock_guard<std::mutex> lock(core_->mutexConfig);
			nlohmann::json config;
			core_->getCurrentConfig(config);
			config.at("plugin").at("installed")[name_] = nlohmann::json::object();
			config.at("plugin").at("installed")[name_]["version"] = installedVersion;
			core_->updateConfig(config);
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
