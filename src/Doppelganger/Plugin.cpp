#ifndef PLUGIN_CPP
#define PLUGIN_CPP

#include "Doppelganger/Plugin.h"

#include <string>
#include <sstream>
#include <fstream>
#if defined(_WIN64)
#include "windows.h"
#include "libloaderapi.h"
#elif defined(__APPLE__)
#include <dlfcn.h>
#elif defined(__linux__)
#include <dlfcn.h>
#endif

#include "Doppelganger/Util/download.h"
#include "Doppelganger/Util/unzip.h"
#include "Doppelganger/Util/log.h"

namespace
{
	////
	// typedef for loading API from dll/lib
#if defined(_WIN64)
	using APIPtr_t = void(__stdcall *)(const char *, const char *, char *, char *, char *);
	using DeallocatePtr_t = void(__stdcall *)();
#elif defined(__APPLE__)
	using APIPtr_t = void (*)(const char *, const char *, char *, char *, char *);
	using DeallocatePtr_t = void (*)();
#elif defined(__linux__)
	using APIPtr_t = void (*)(const char *, const char *, char *, char *, char *);
	using DeallocatePtr_t = void (*)();
#endif

	void functionCall(
		const fs::path &dllPath,
		const std::string &functionName,
		nlohmann::json &config,
		const nlohmann::json &parameter,
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
			FARPROC pluginFunc = GetProcAddress(handle, functionName.c_str());
			FARPROC deallocateFunc = GetProcAddress(handle, "deallocate");
#elif defined(__APPLE__)
			void *pluginFunc = dlsym(handle, functionName.c_str());
			void *deallocateFunc = dlsym(handle, "deallocate");
#elif defined(__linux__)
			void *pluginFunc = dlsym(handle, functionName.c_str());
			void *deallocateFunc = dlsym(handle, "deallocate");
#endif
			if (pluginFunc && deallocateFunc)
			{
				// setup buffers
				const char *configChar = config.dump().c_str();
				const char *parameterChar = parameter.dump().c_str();
				char *modifiedConfigChar = nullptr;
				char *responseChar = nullptr;
				char *broadcastChar = nullptr;
				// pluginFunc
				reinterpret_cast<APIPtr_t>(pluginFunc)(configChar, parameterChar, modifiedConfigChar, responseChar, broadcastChar);
				// read results (todo: check, does parse() performs deep copy?)
				// todo: do we use .update() for config?
				config = nlohmann::json::parse(modifiedConfigChar);
				response.update(nlohmann::json::parse(responseChar));
				broadcast.update(nlohmann::json::parse(broadcastChar));
				// deallocation
				reinterpret_cast<DeallocatePtr_t>(deallocateFunc)();
			}
#if defined(_WIN64)
			FreeLibrary(handle);
#elif defined(__APPLE__)
			dlclose(handle);
#elif defined(__linux__)
			dlclose(handle);
#endif
		}
	}
}

namespace Doppelganger
{
	Plugin::Plugin(
		const std::string &name,
		const nlohmann::json &parameters)
		: name_(name), parameters_(parameters)
	{
	}

	void Plugin::install(
		nlohmann::json &config,
		const std::string &version)
	{
		const std::string actualVersion((version == "latest") ? parameters_.at("versions").at(0).at("version").get<std::string>() : version);

		pluginDir_ = fs::path(config.at("plugin").at("dir").get<std::string>());
		std::string dirName("");
		dirName += name_;
		dirName += "_";
		dirName += actualVersion;
		pluginDir_.append(dirName);

		if (!fs::exists(pluginDir_))
		{
			bool versionFound = false;
			for (const auto &versionEntry : parameters_.at("versions"))
			{
				const std::string &pluginVersion = versionEntry.at("version").get<std::string>();
				if (pluginVersion == actualVersion)
				{
					const std::string &pluginURL = versionEntry.at("URL").get<std::string>();
					fs::path zipPath(config.at("plugin").at("dir").get<std::string>());
					zipPath.append("tmp.zip");
					if (Util::download(pluginURL, zipPath))
					{
						Util::unzip(zipPath, pluginDir_);
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
						Util::log(ss.str(), "ERROR", config);
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
				Util::log(ss.str(), "ERROR", config);
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
				Util::log(ss.str(), "SYSTEM", config);
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
			Util::log(ss.str(), "SYSTEM", config);
		}
		installedVersion_ = version;

		// javascript module (if exists)
		{
			const std::string moduleName("module.js");
			fs::path modulePath(pluginDir_);
			modulePath.append(moduleName);
			hasModuleJS_ = fs::exists(modulePath);
		}

		// update installed plugin list "installed.json"
		{
			const int priority = config.at("plugin").at("installed").size();
			config.at("plugin").at("installed")[name_] = nlohmann::json::object();
			config.at("plugin").at("installed")[name_]["version"] = installedVersion_;
			config.at("plugin").at("installed")[name_]["priority"] = priority;
		}
	}

	void Plugin::pluginProcess(
		nlohmann::json &configCore,
		nlohmann::json &configRoom,
		nlohmann::json &configMesh,
		const nlohmann::json &parameters,
		nlohmann::json &response,
		nlohmann::json &broadcast)
	{
		fs::path dllPath(pluginDir_);
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
			functionCall(dllPath, "pluginProcessCore", configCore, parameters, response, broadcast);
			functionCall(dllPath, "pluginProcessRoom", configRoom, parameters, response, broadcast);
			functionCall(dllPath, "pluginProcessMesh", configMesh, parameters, response, broadcast);
		}
	}

	void Plugin::getCatalogue(
		const nlohmann::json &config,
		nlohmann::json &catalogue)
	{
		catalogue = nlohmann::json::object();

		for (const auto &listUrl : config.at("plugin").at("listURL"))
		{
			fs::path listJsonPath(config.at("plugin").at("dir").get<std::string>());
			listJsonPath.append("tmp.json");
			Util::download(listUrl.get<std::string>(), listJsonPath);
			std::ifstream ifs(listJsonPath.string());
			if (ifs)
			{
				catalogue.update(nlohmann::json::parse(ifs));
				ifs.close();
				fs::remove_all(listJsonPath);
			}
		}
	}
} // namespace

#endif
