#ifndef PLUGIN_CPP
#define PLUGIN_CPP

#include "Doppelganger/Plugin.h"
#include "Doppelganger/Core.h"
#include "Doppelganger/Util/download.h"
#include "Doppelganger/Util/unzip.h"
#include <string>
#include <fstream>
#include <streambuf>
#if defined(_WIN32) || defined(_WIN64)
#define STDCALL __stdcall
#include "windows.h"
#include "libloaderapi.h"
#elif defined(__APPLE__)
#include <dlfcn.h>
#endif

namespace
{
	////
	// typedef for loading API from dll/lib
	typedef void(STDCALL *APIPtr_t)(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &, nlohmann::json &, nlohmann::json &);

	bool loadDll(const fs::path &dllPath, const std::string &functionName, Doppelganger::Plugin::API_t &apiFunc)
	{
#if defined(_WIN32) || defined(_WIN64)
		HINSTANCE handle = LoadLibrary(dllPath.string().c_str());
#else
		void *handle;
		handle = dlopen(dllPath.string().c_str(), RTLD_LAZY);
#endif

		if (handle != NULL)
		{
#if defined(_WIN32) || defined(_WIN64)
			FARPROC lpfnDllFunc = GetProcAddress(handle, functionName.c_str());
#else
			void *lpfnDllFunc = dlsym(handle, functionName.c_str());
#endif
			if (!lpfnDllFunc)
			{
				// error
#if defined(_WIN32) || defined(_WIN64)
				FreeLibrary(handle);
#else
				dlclose(handle);
#endif
				return false;
			}
			else
			{
				apiFunc = Doppelganger::Plugin::API_t(reinterpret_cast<APIPtr_t>(lpfnDllFunc));
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
	Plugin::Plugin(const std::shared_ptr<Core> &core_, const std::string &name_, const nlohmann::json &parameters_)
		: core(core_), name(name_), parameters(parameters_), hasModuleJS(false)
	{
	}

	void Plugin::install(const std::string &version)
	{
		const std::string actualVersion((version == "latest") ? parameters.at("versions").at(0).at("version").get<std::string>() : version);

		fs::path pluginsDir(core->config.at("plugin").at("dir").get<std::string>());
		fs::path pluginDir(pluginsDir);
		std::string dirName(name);
		dirName += "_";
		dirName += actualVersion;
		pluginDir.append(dirName);
		if (!fs::exists(pluginDir))
		{
			bool versionFound = false;
			for (const auto &versionEntry : parameters.at("versions"))
			{
				const std::string &pluginVersion = versionEntry.at("version").get<std::string>();
				if (pluginVersion == actualVersion)
				{
					const std::string &pluginURL = versionEntry.at("URL").get<std::string>();
					fs::path zipPath(pluginsDir);
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
						ss << "Plugin \"";
						ss << name;
						ss << "\" (";
						if (version == "latest")
						{
							ss << "latest, ";
						}
						ss << actualVersion;
						ss << ")";
						ss << " is NOT loaded correctly. (Download)";
						core->logger.log(ss.str(), "ERROR");
						return;
					}
				}
			}

			if (!versionFound)
			{
				// failure
				std::stringstream ss;
				ss << "Plugin \"";
				ss << name;
				ss << "\" (";
				if (version == "latest")
				{
					ss << "latest, ";
				}
				ss << actualVersion;
				ss << ")";
				ss << " is NOT loaded correctly. (No such version)";
				core->logger.log(ss.str(), "ERROR");
				return;
			}
		}
		else
		{
			// already downloaded
			std::stringstream ss;
			ss << "Plugin \"";
			ss << name;
			ss << "\" (";
			if (version == "latest")
			{
				ss << "latest, ";
			}
			ss << actualVersion;
			ss << ")";
			ss << " is already downloaded. We reuse it.";
			core->logger.log(ss.str(), "SYSTEM");
		}
		installedVersion = version;
		installFromDirectory(pluginDir);
	}

	void Plugin::installFromDirectory(const fs::path &pluginDir)
	{
		std::string dllName(name);
#if defined(_WIN32) || defined(_WIN64)
		dllName += ".dll";
#else
		dllName += ".so";
#endif
		// c++ functions (.dll/.so) (if exists)
		fs::path dllPath(pluginDir);
		dllPath.append(dllName);
		if (fs::exists(dllPath))
		{
			if (!loadDll(dllPath, "pluginProcess", func))
			{
				std::stringstream ss;
				ss << "Plugin \"";
				ss << name;
				ss << "\" (";
				ss << installedVersion;
				ss << ")";
				ss << " is NOT loaded correctly. (Function pluginProcess not found)";
				core->logger.log(ss.str(), "ERROR");
				return;
			}
		}

		// javascript module (if exists)
		std::string moduleName("module.js");
		fs::path modulePath(pluginDir);
		modulePath.append(moduleName);
		hasModuleJS = fs::exists(modulePath);

		// Plugin "<pluginName>" (<Version>) is loaded.
		{
			std::stringstream ss;
			ss << "Plugin \"";
			ss << name;
			ss << "\" (";
			ss << installedVersion;
			ss << ")";
			ss << " is loaded.";
			core->logger.log(ss.str(), "SYSTEM");
		}

		// update installed plugin list "installed.json"
		{
			fs::path installedPluginJsonPath(core->config.at("plugin").at("dir").get<std::string>());
			installedPluginJsonPath.append("installed.json");

			nlohmann::json installedPluginJson;

			std::ifstream ifs(installedPluginJsonPath);
			installedPluginJson = nlohmann::json::parse(ifs);
			ifs.close();

			nlohmann::json pluginInfo = nlohmann::json::object();
			pluginInfo["name"] = name;
			pluginInfo["version"] = installedVersion;
			installedPluginJson.push_back(pluginInfo);

			std::ofstream ofs(installedPluginJsonPath);
			ofs << installedPluginJson.dump(4);
			ofs.close();
		}
	}
} // namespace

#endif
