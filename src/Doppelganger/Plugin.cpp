#ifndef PLUGIN_CPP
#define PLUGIN_CPP

#include "Doppelganger/Plugin.h"
#include "Doppelganger/Core.h"
#include "Util/download.h"
#include "Util/unzip.h"
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
	Plugin::Plugin(const std::shared_ptr<Core> &core_)
		: core(core_)
	{
	}

	bool Plugin::loadPlugin(const std::string &name, const std::string &pluginUrl)
	{
		// path to downloaded zip file
		fs::path zipPath(core->systemParams.workingDir);
		zipPath.append("plugin");
		zipPath.append("tmp.zip");
		if (Util::download(pluginUrl, zipPath))
		{
			// path to extracted directory
			fs::path pluginDir(core->config.at("plugin").at("dir").get<std::string>());
			pluginDir.append(name);
			Util::unzip(zipPath, pluginDir);
			// erase temporary file
			fs::remove_all(zipPath);

			// load plugin from extracted directory
			return loadPlugin(pluginDir);
		}
		return false;
	}

	bool Plugin::loadPlugin(const fs::path &pluginDir)
	{
		name = pluginDir.stem().string();
		std::string dllName(name);
#if defined(_WIN32) || defined(_WIN64)
		dllName += ".dll";
#else
		dllName += ".so";
#endif
		// c++ functions
		fs::path dllPath(pluginDir);
		dllPath.append(dllName);
		if (!loadDll(dllPath, "pluginProcess", func))
		{
			return false;
		}

		API_t metadataFunc;
		if (!loadDll(dllPath, "metadata", metadataFunc))
		{
			return false;
		}
		nlohmann::json parameters, response, broadcast;
		// dummy parameters...
		metadataFunc(nullptr, parameters, response, broadcast);
		author = response.at("author").get<std::string>();
		version = response.at("version").get<std::string>();

		// Plugin "<pluginName>" (<Version>, <Author>) is loaded.
		{
			std::stringstream ss;
			ss << "Plugin \"";
			ss << name;
			ss << "\" (";
			ss << version;
			ss << ", ";
			ss << author;
			ss << ")";
			ss << " is loaded.";
			core->logger.log(ss.str(), "SYSTEM");
		}

		// javascript module (if exists)
		std::string moduleName(name);
		moduleName += ".js";
		fs::path modulePath(pluginDir);
		modulePath.append(moduleName);
		if (fs::exists(modulePath))
		{
			std::ifstream ifs(modulePath);
			moduleJS = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
		}

		return true;
	}
} // namespace

#endif
