#ifndef PLUGIN_CPP
#define PLUGIN_CPP

#include "Doppelganger/Plugin.h"
#include "Doppelganger/Core.h"
#include "Doppelganger/Room.h"

#include <sstream>
#if defined(_WIN64)
#include "windows.h"
#include "libloaderapi.h"
#elif defined(__APPLE__)
#include <dlfcn.h>
#elif defined(__linux__)
#include <dlfcn.h>
#endif

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
	void Plugin::install(
		const std::weak_ptr<Room> &room,
		const std::string &version)
	{

		const std::string actualVersion((version == "latest") ? versions_.at(0).version : version);

		std::string dirName("");
		dirName += name_;
		dirName += "_";
		dirName += actualVersion;

		fs::path cachedDir(room.lock()->DoppelgangerRootDir_);
		cachedDir.append("plugin");
		cachedDir.append(dirName);

		dir_ = fs::path(room.lock()->dataDir_);
		dir_.append("plugin");
		dir_.append(dirName);

		if (!fs::exists(cachedDir))
		{
			bool versionFound = false;
			for (const auto &versionEntry : versions_)
			{
				const std::string &pluginVersion = versionEntry.version;
				if (pluginVersion == actualVersion)
				{
					const std::string &pluginURL = versionEntry.URL;
					fs::path zipPath(room.lock()->DoppelgangerRootDir_);
					zipPath.append("plugin");
					zipPath.append("tmp.zip");
					if (Util::download(pluginURL, zipPath))
					{
						Util::unzip(zipPath, cachedDir);
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
						   << " is NOT downloaded correctly. (Download)";
						Util::log(ss.str(), "ERROR", room.lock()->dataDir_, room.lock()->logConfig_);
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
				Util::log(ss.str(), "ERROR", room.lock()->dataDir_, room.lock()->logConfig_);
				// remove invalid dir_
				dir_ = fs::path();
				return;
			}
		}
		installedVersion_ = version;

		// copy cached plugin into room
		fs::copy(cachedDir, dir_, fs::copy_options::recursive);
		{
			std::stringstream ss;
			ss << "Plugin \"" << name_ << "\" (";
			if (version == "latest")
			{
				ss << "latest, ";
			}
			ss << actualVersion << ")"
			   << " is loaded.";
			Util::log(ss.str(), "SYSTEM", room.lock()->dataDir_, room.lock()->logConfig_);
		}

		room.lock()->installedPlugin_.push_back({name_, installedVersion_});
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
			plugin.versions_.push_back(Plugin::VersionResourceInfo({version, URL}));
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
