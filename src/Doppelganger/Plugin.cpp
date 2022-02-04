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
	void getPtrStrArrayForPartialConfig(
		const fs::path &dllPath,
		const char *parameterChar,
		nlohmann::json &ptrStrArrayCore,
		nlohmann::json &ptrStrArrayRoom);
	void functionCall(
		const fs::path &dllPath,
		const std::string &functionName,
		const nlohmann::json &configCore,
		const nlohmann::json &configRoom,
		const nlohmann::json &parameter,
		nlohmann::json &configCorePatch,
		nlohmann::json &configRoomPatch,
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

		fs::path cachedDir(room.lock()->config.at("DoppelgangerRootDir").get<std::string>());
		cachedDir.append("plugin");
		cachedDir.append(dirName);

		dir_ = fs::path(room.lock()->config.at("dataDir").get<std::string>());
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
					fs::path zipPath(room.lock()->config.at("DoppelgangerRootDir").get<std::string>());
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
						Util::log(ss.str(), "ERROR", room.lock()->config);
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
				Util::log(ss.str(), "ERROR", room.lock()->config);
				// remove invalid dir_
				dir_ = fs::path();
				return;
			}
		}

		if (!fs::exists(dir_))
		{
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
				Util::log(ss.str(), "SYSTEM", room.lock()->config);
			}
		}

		installedVersion_ = version;
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
			nlohmann::json configCorePatch, configRoomPatch;
			functionCall(dllPath, "pluginProcess", core.lock()->config, room.lock()->config, parameters, configCorePatch, configRoomPatch, response, broadcast);
			if (!configRoomPatch.is_null())
			{
				room.lock()->config.merge_patch(configRoomPatch);
				room.lock()->applyCurrentConfig();
			}
			if (!configCorePatch.is_null())
			{
				core.lock()->config.merge_patch(configCorePatch);
				core.lock()->applyCurrentConfig();
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
		json["hasModuleJS"] = plugin.hasModuleJS_;
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
		plugin.hasModuleJS_ = json.at("hasModuleJS").get<bool>();
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
	void getPtrStrArrayForPartialConfig(
#if defined(_WIN64)
		HINSTANCE handle,
#elif defined(__APPLE__)
		void *handle,
#elif defined(__linux__)
		void *handle,
#endif
		const char *parameterChar,
		nlohmann::json &ptrStrArrayCore,
		nlohmann::json &ptrStrArrayRoom)
	{
		// by default, we request all config (this setting would be overwritten below)
		ptrStrArrayCore = nlohmann::json::array({"/"});
		ptrStrArrayRoom = nlohmann::json::array({"/"});

#if defined(_WIN64)
		FARPROC pluginFunc = GetProcAddress(handle, "getPtrStrArrayForPartialConfig");
#elif defined(__APPLE__)
		void *pluginFunc = dlsym(handle, "getPtrStrArrayForPartialConfig");
#elif defined(__linux__)
		void *pluginFunc = dlsym(handle, "getPtrStrArrayForPartialConfig");
#endif
		if (pluginFunc)
		{
			// setup buffers
			char *ptrStrArrayCoreChar = nullptr;
			char *ptrStrArrayRoomChar = nullptr;
			// pluginFunc
#if defined(_WIN64)
			using APIPtr_t = void(__stdcall *)(const char *&, char *&, char *&);
#elif defined(__APPLE__)
			using APIPtr_t = void (*)(const char *&, char *&, char *&);
#elif defined(__linux__)
			using APIPtr_t = void (*)(const char *&, char *&, char *&);
#endif
			reinterpret_cast<APIPtr_t>(pluginFunc)(
				parameterChar,
				ptrStrArrayCoreChar,
				ptrStrArrayRoomChar);
			if (ptrStrArrayCoreChar != nullptr)
			{
				ptrStrArrayCore = nlohmann::json::parse(ptrStrArrayCoreChar);
			}
			if (ptrStrArrayRoomChar != nullptr)
			{
				ptrStrArrayRoom = nlohmann::json::parse(ptrStrArrayRoomChar);
			}
		}
	}

	void functionCall(
		const fs::path &dllPath,
		const std::string &functionName,
		const nlohmann::json &configCore,
		const nlohmann::json &configRoom,
		const nlohmann::json &parameter,
		nlohmann::json &configCorePatch,
		nlohmann::json &configRoomPatch,
		nlohmann::json &response,
		nlohmann::json &broadcast)
	{
#if defined(_WIN64)
		using APIPtr_t = void(__stdcall *)(const char *&, const char *&, const char *&, char *&, char *&, char *&, char *&);
		using DeallocatePtr_t = void(__stdcall *)();
		HINSTANCE handle = LoadLibrary(dllPath.string().c_str());
#elif defined(__APPLE__)
		using APIPtr_t = void (*)(const char *&, const char *&, const char *&, char *&, char *&, char *&, char *&);
		using DeallocatePtr_t = void (*)();
		void *handle;
		handle = dlopen(dllPath.string().c_str(), RTLD_LAZY);
#elif defined(__linux__)
		using APIPtr_t = void (*)(const char *&, const char *&, const char *&, char *&, char *&, char *&, char *&);
		using DeallocatePtr_t = void (*)();
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
				const std::string parameterStr = parameter.dump(-1, ' ', true);
				const char *parameterChar = parameterStr.c_str();

				nlohmann::json ptrStrArrayCore, ptrStrArrayRoom;
				getPtrStrArrayForPartialConfig(
					handle,
					parameterChar,
					ptrStrArrayCore,
					ptrStrArrayRoom);

				nlohmann::json partialConfigCore, partialConfigRoom;
				partialConfigCore = nlohmann::json::object();
				for (const auto &ptrStrJson : ptrStrArrayCore)
				{
					const nlohmann::json::json_pointer ptr(ptrStrJson.get<std::string>());
					// we explicitly check by using .contains()
					//     e.g. ptr == "/extension/plugin_A/...", but config.at("extension")("plugin_A") == null
					if (configCore.contains(ptr))
					{
						partialConfigCore[ptr] = configCore.at(ptr);
					}
				}
				partialConfigRoom = nlohmann::json::object();
				for (const auto &ptrStrJson : ptrStrArrayRoom)
				{
					const nlohmann::json::json_pointer ptr(ptrStrJson.get<std::string>());
					if (configRoom.contains(ptr))
					{
						partialConfigRoom[ptr] = configRoom.at(ptr);
					}
				}

				const std::string configCoreStr = partialConfigCore.dump(-1, ' ', true);
				const char *configCoreChar = configCoreStr.c_str();
				const std::string configRoomStr = partialConfigRoom.dump(-1, ' ', true);
				const char *configRoomChar = configRoomStr.c_str();
				char *configCorePatchChar = nullptr;
				char *configRoomPatchChar = nullptr;
				char *responseChar = nullptr;
				char *broadcastChar = nullptr;
				// pluginFunc
				reinterpret_cast<APIPtr_t>(pluginFunc)(
					configCoreChar,
					configRoomChar,
					parameterChar,
					configCorePatchChar,
					configRoomPatchChar,
					responseChar,
					broadcastChar);
				if (configCorePatchChar != nullptr)
				{
					configCorePatch = nlohmann::json::parse(configCorePatchChar);
				}
				if (configRoomPatchChar != nullptr)
				{
					configRoomPatch = nlohmann::json::parse(configRoomPatchChar);
				}
				// we respond something (for example, empty json object)
				if (responseChar != nullptr)
				{
					response = nlohmann::json::parse(responseChar);
				}
				else
				{
					response = nlohmann::json::object();
				}
				// broadcast could be null
				if (broadcastChar != nullptr)
				{
					broadcast = nlohmann::json::parse(broadcastChar);
				}

				// deallocate memory malloc-ed within dll
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
