#ifndef PLUGIN_CPP
#define PLUGIN_CPP

#include "Doppelganger/Plugin.h"
#include "Doppelganger/Core.h"
#include "Doppelganger/Room.h"

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
	void functionCall(
		const fs::path &dllPath,
		const std::string &functionName,
		nlohmann::json &configCore,
		nlohmann::json &configRoom,
		const nlohmann::json &parameter,
		nlohmann::json &response,
		nlohmann::json &broadcast);
}

namespace Doppelganger
{
	Plugin::Plugin()
	{
		// for nlohmann::json conversion, Plugin is default constructable.
		// for actual use, we ALWAYS use nlohmann::json conversion for initialize.
	}

	void Plugin::install(
		const CoreRoom &coreRoom,
		const std::string &version)
	{
		const auto installFunc = [this, &version](const auto &cr)
		{
			const std::string actualVersion((version == "latest") ? versions_.at(0).version : version);

			dir_ = fs::path(cr.lock()->DoppelgangerRootDir_);
			dir_.append("plugin");
			std::string dirName("");
			dirName += name_;
			dirName += "_";
			dirName += actualVersion;
			dir_.append(dirName);

			if (!fs::exists(dir_))
			{
				bool versionFound = false;
				for (const auto &versionEntry : versions_)
				{
					const std::string &pluginVersion = versionEntry.version;
					if (pluginVersion == actualVersion)
					{
						const std::string &pluginURL = versionEntry.URL;
						fs::path zipPath(cr.lock()->DoppelgangerRootDir_);
						zipPath.append("plugin");
						zipPath.append("tmp.zip");
						if (Util::download(pluginURL, zipPath))
						{
							Util::unzip(zipPath, dir_);
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
							Util::log(ss.str(), "ERROR", cr.lock()->dataDir_, cr.lock()->logConfig_.level, cr.lock()->logConfig_.type);
							// remove invalid dir_
							dir_ = fs::path();
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
					Util::log(ss.str(), "ERROR", cr.lock()->dataDir_, cr.lock()->logConfig_.level, cr.lock()->logConfig_.type);
					// remove invalid dir_
					dir_ = fs::path();
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
					Util::log(ss.str(), "SYSTEM", cr.lock()->dataDir_, cr.lock()->logConfig_.level, cr.lock()->logConfig_.type);
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
				Util::log(ss.str(), "SYSTEM", cr.lock()->dataDir_, cr.lock()->logConfig_.level, cr.lock()->logConfig_.type);
			}
			installedVersion_ = version;
		};

		std::visit(installFunc, coreRoom);
	}

	void Plugin::pluginProcess(
		const std::weak_ptr<Core> &core,
		const std::weak_ptr<Room> &room,
		const nlohmann::json &parameters,
		nlohmann::json &response,
		nlohmann::json &broadcast)
	{
		fs::path dllPath(dir_);
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
			nlohmann::json configCore, configRoom;
			core.lock()->to_json(configCore);
			room.lock()->to_json(configRoom);
			functionCall(dllPath, "pluginProcess", configCore, configRoom, parameters, response, broadcast);
			// we need to optimize here...
			room.lock()->from_json(configRoom);
			room.lock()->applyCurrentConfig();
			core.lock()->from_json(configCore);
			core.lock()->applyCurrentConfig();
		}
	}

	void Plugin::getCatalogue(
		const fs::path &pluginDir,
		const std::vector<std::string> &listURLList,
		nlohmann::json &catalogue)
	{
		catalogue = nlohmann::json::array();

		for (const auto &listURL : listURLList)
		{
			fs::path listJsonPath(pluginDir);
			listJsonPath.append("tmp.json");
			Util::download(listURL, listJsonPath);
			std::ifstream ifs(listJsonPath.string());
			if (ifs)
			{
				nlohmann::json list = nlohmann::json::parse(ifs);
				ifs.close();
				fs::remove_all(listJsonPath);
				catalogue.insert(catalogue.end(), list.begin(), list.end());
			}
		}
	}

	////
	// nlohmann::json conversion
	////
	void to_json(nlohmann::json &json, const Plugin &plugin)
	{
		json = nlohmann::json::object();
		json["name"] = plugin.name_;
		json["description"] = nlohmann::json::object();
		for (const auto &lang_description : plugin.description_)
		{
			const std::string &lang = lang_description.first;
			const std::string &description = lang_description.second;
			json["description"][lang] = description;
		}
		json["optional"] = plugin.optional_;
		json["UIPosition"] = plugin.UIPosition_;
		json["versions"] = nlohmann::json::array();
		for (const auto &versionInfo : plugin.versions_)
		{
			nlohmann::json versionJson = nlohmann::json::object();
			versionJson["version"] = versionInfo.version;
			versionJson["URL"] = versionInfo.URL;
			json["versions"].push_back(versionJson);
		}
		json["installedVersion"] = plugin.installedVersion_;
		json["dir"] = plugin.dir_.string();
	}

	void from_json(const nlohmann::json &json, Plugin &plugin)
	{
		plugin.name_ = json.at("name").get<std::string>();
		plugin.description_.clear();
		for (const auto &lang_description : json.at("description").items())
		{
			const std::string &lang = lang_description.key();
			const std::string description = lang_description.value().get<std::string>();
			plugin.description_[lang] = description;
		}
		plugin.optional_ = json.at("optional").get<bool>();
		plugin.UIPosition_ = json.at("UIPosition").get<std::string>();
		plugin.versions_.clear();
		for (const auto &versionInfo : json.at("versions"))
		{
			const std::string version = versionInfo.at("version").get<std::string>();
			const std::string URL = versionInfo.at("URL").get<std::string>();
			plugin.versions_.push_back(Plugin::VersionInfo({version, URL}));
		}
		if (json.contains("installedVersion"))
		{
			plugin.installedVersion_ = json.at("installedVersion").get<std::string>();
		}
		else
		{
			plugin.installedVersion_ = std::string("");
		}
		if (json.contains("dir"))
		{
			plugin.dir_ = fs::path(json.at("dir").get<std::string>());
		}
		else
		{
			plugin.dir_ = fs::path();
		}
	}
}

namespace
{
	////
	// typedef for loading API from dll/lib
#if defined(_WIN64)
	using APIPtr_t = void(__stdcall *)(const char *, const char *, const char *, char *, char *, char *, char *);
	using DeallocatePtr_t = void(__stdcall *)();
#elif defined(__APPLE__)
	using APIPtr_t = void (*)(const char *, const char *, const char *, char *, char *, char *, char *);
	using DeallocatePtr_t = void (*)();
#elif defined(__linux__)
	using APIPtr_t = void (*)(const char *, const char *, const char *, char *, char *, char *, char *);
	using DeallocatePtr_t = void (*)();
#endif

	void functionCall(
		const fs::path &dllPath,
		const std::string &functionName,
		nlohmann::json &configCore,
		nlohmann::json &configRoom,
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
				const char *configCoreChar = configCore.dump().c_str();
				const char *configRoomChar = configRoom.dump().c_str();
				const char *parameterChar = parameter.dump().c_str();
				char *modifiedConfigCoreChar = nullptr;
				char *modifiedConfigRoomChar = nullptr;
				char *responseChar = nullptr;
				char *broadcastChar = nullptr;
				// pluginFunc
				reinterpret_cast<APIPtr_t>(pluginFunc)(
					configCoreChar,
					configRoomChar,
					parameterChar,
					modifiedConfigCoreChar,
					modifiedConfigRoomChar,
					responseChar,
					broadcastChar);
				if (modifiedConfigCoreChar != nullptr)
				{
					configCore.update(nlohmann::json::parse(modifiedConfigCoreChar));
				}
				if (modifiedConfigRoomChar != nullptr)
				{
					configRoom.update(nlohmann::json::parse(modifiedConfigRoomChar));
				}
				if (responseChar != nullptr)
				{
					response = nlohmann::json::parse(responseChar);
				}
				if (broadcastChar != nullptr)
				{
					broadcast = nlohmann::json::parse(broadcastChar);
				}
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

#endif
