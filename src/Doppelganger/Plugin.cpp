#ifndef PLUGIN_CPP
#define PLUGIN_CPP

#include "Plugin.h"
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
	typedef void(STDCALL *APIPtr_t)(const std::shared_ptr<Doppelganger::Room> &, const nlohmann::json &, nlohmann::json &);

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
	Plugin::Plugin(const fs::path &pluginsDir_)
		: pluginsDir(pluginsDir_)
	{
	}

	void Plugin::loadPlugin(const std::string &pluginUrl)
	{
		// download zip
		// todo

		fs::path pluginDir;
		loadPlugin(pluginDir);
	}
	void Plugin::loadPlugin(const fs::path &pluginDir)
	{
		name = pluginDir.stem().string();
		std::string dllName(name);
#if defined(_WIN32) || defined(_WIN64)
		dllName += ".dll";
#else
		dllName += ".so";
#endif
		std::string moduleName(name);
		moduleName += ".js";
		fs::path dllPath(pluginDir);
		dllPath.append(dllName);
		loadDll(dllPath, "pluginProcess", func);

		fs::path modulePath(pluginDir);
		modulePath.append(moduleName);
		std::ifstream ifs(modulePath);
		moduleJS = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());

		API_t infoFunc;
		loadDll(dllPath, "info", infoFunc);
		nlohmann::json parameters, response;
		// dummy parameters...
		infoFunc(nullptr, parameters, response);
		author = response.at("author").get<std::string>();
		version = response.at("version").get<std::string>();
	}

} // namespace

#endif
