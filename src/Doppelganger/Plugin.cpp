#ifndef PLUGIN_CPP
#define PLUGIN_CPP

#include "Plugin.h"
#include "Core.h"
#include <fstream>
#include <streambuf>
#if defined(_WIN32) || defined(_WIN64)
#define STDCALL __stdcall
#include "windows.h"
#include "libloaderapi.h"
#elif defined(__APPLE__)
#include <dlfcn.h>
#endif
#include "minizip/unzip.h"
#include "boost/beast/core.hpp"
#include "boost/beast/http.hpp"

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
	Plugin::Plugin(const std::shared_ptr<Core> &core_)
		: core(core_)
	{
	}

	void Plugin::loadPlugin(const std::string &name, const std::string &pluginUrl)
	{
		// path to downloaded zip file
		fs::path zipPath;
		// https://www.boost.org/doc/libs/1_77_0/libs/beast/example/http/client/sync/http_client_sync.cpp
		try
		{
			std::string host("");
			// 		auto const host = argv[1];
			// 		auto const port = argv[2];
			// 		auto const target = argv[3];
			// 		int version = argc == 5 && !std::strcmp("1.0", argv[4]) ? 10 : 11;

			// 		net::io_context ioc;
			// 		tcp::resolver resolver(ioc);
			// 		beast::tcp_stream stream(ioc);

			// 		auto const results = resolver.resolve(host, port);
			// 		stream.connect(results);

			// 		http::request<http::string_body> req{http::verb::get, target, version};
			// 		req.set(http::field::host, host);
			// 		req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

			// 		http::write(stream, req);

			// 		beast::flat_buffer buffer;
			// 		http::response<http::dynamic_body> res;
			// 		http::read(stream, buffer, res);

			// 		// Write the message to standard out
			// 		std::cout << res << std::endl;

			// 		beast::error_code ec;
			// 		stream.socket().shutdown(tcp::socket::shutdown_both, ec);

			// 		// not_connected happens sometimes
			// 		// so don't bother reporting it.
			// 		//
			// 		if (ec && ec != beast::errc::not_connected)
			// 		{
			// 			throw beast::system_error{ec};
			// 		}
		}
		catch (std::exception const &e)
		{
			std::stringstream err;
			err << "Error: " << e.what();
			core->logger.log(err.str(), "ERROR");
		}
		////
		// path to extracted directory
		fs::path pluginDir(core->config.at("pluginDir").get<std::string>());
		pluginDir.append(name);
		unzip(zipPath, pluginDir);
		// load plugin from extracted directory
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
		fs::path dllPath(pluginDir);
		dllPath.append(dllName);
		loadDll(dllPath, "pluginProcess", func);

		std::string moduleName(name);
		moduleName += ".js";
		fs::path modulePath(pluginDir);
		modulePath.append(moduleName);
		if (fs::exists(modulePath))
		{
			std::ifstream ifs(modulePath);
			moduleJS = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
		}

		API_t metadataFunc;
		loadDll(dllPath, "metadata", metadataFunc);
		nlohmann::json parameters, response;
		// dummy parameters...
		metadataFunc(nullptr, parameters, response);
		author = response.at("author").get<std::string>();
		version = response.at("version").get<std::string>();
	}

	void Plugin::unzip(const fs::path &zipPath, const fs::path &destPath)
	{
		// http://hp.vector.co.jp/authors/VA016379/cpplib/zip.cpp
		unzFile unzipHandle = unzOpen(zipPath.string().c_str());
		if (unzipHandle)
		{
			do
			{
				char fileName[512];
				unz_file_info fileInfo;
				if (unzGetCurrentFileInfo(unzipHandle, &fileInfo, fileName, sizeof fileName, NULL, 0, NULL, 0) != UNZ_OK)
				{
					break;
				}

				fs::path filePath(fileName);
				filePath.make_preferred();
				if (fs::is_directory(filePath))
				{
					// create direcotry
					fs::path targetPath(destPath);
					for (const auto &p : filePath)
					{
						targetPath.append(p.string());
					}
					fs::create_directories(targetPath);
					continue;
				}

				if (unzOpenCurrentFile(unzipHandle) != UNZ_OK)
				{
					break;
				}
				if (fs::is_regular_file(filePath))
				{
					// create direcotry
					fs::path targetPath(destPath);
					for (const auto &p : filePath)
					{
						targetPath.append(p.string());
					}
					fs::create_directories(targetPath.parent_path());

					std::ofstream ofs(targetPath, std::ios::binary);

					char buffer[8192];
					unsigned long sizeRead;
					while ((sizeRead = unzReadCurrentFile(unzipHandle, buffer, sizeof buffer)) > 0)
					{
						ofs.write(buffer, sizeRead);
					}
					ofs.close();
					unzCloseCurrentFile(unzipHandle);
				}
			} while (unzGoToNextFile(unzipHandle) != UNZ_END_OF_LIST_OF_FILE);

			unzClose(unzipHandle);
		}
	}
} // namespace

#endif
