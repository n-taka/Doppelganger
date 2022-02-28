#ifndef CORE_CPP
#define CORE_CPP

#include "Doppelganger/Core.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/HTTPSession.h"
#include "Doppelganger/Plugin.h"
#include "Doppelganger/Listener.h"
#include "Doppelganger/Util/getCurrentTimestampAsString.h"
#include "Doppelganger/Util/getPluginCatalogue.h"
#include "Doppelganger/Util/log.h"

#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <cstddef>
#include <memory>

#include <fstream>
#include <sstream>
#include <iostream>
#include <nlohmann/json.hpp>

#if defined(_WIN64)
#include <shlobj_core.h>
#elif defined(__APPLE__)
#include "CoreFoundation/CoreFoundation.h"
#include "sysdir.h"
#include <stdlib.h>
namespace
{
	// https://stackoverflow.com/questions/4891006/how-to-create-a-folder-in-the-home-directory
	std::string expand_user(std::string path)
	{
		if (!path.empty() && path[0] == '~')
		{
			char const *home = getenv("HOME");
			if (home || (home = getenv("USERPROFILE")))
			{
				path.replace(0, 1, home);
			}
			else
			{
				char const *hdrive = getenv("HOMEDRIVE"),
						   *hpath = getenv("HOMEPATH");
				path.replace(0, 1, std::string(hdrive) + hpath);
			}
		}
		return path;
	}
}
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#include <cstdlib>
#include <sys/types.h>
#include <pwd.h>
#endif

// https://github.com/boostorg/beast/blob/develop/example/http/server/async/http_server_async.cpp

namespace Doppelganger
{
	Core::Core(boost::asio::io_context &ioc,
			   boost::asio::ssl::context &ctx)
		: ioc_(ioc), ctx_(ctx)
	{
	}

	void Core::setup()
	{
		config = nlohmann::json::object();

		// path for DoppelgangerRoot
		{
			fs::path DoppelgangerRootDir_;
#if defined(_WIN64)
			// For windows, we use "%USERPROFILE%\AppData\Local\Doppelganger"
			// e.g. "C:\Users\KazutakaNakashima\AppData\Local\Doppelganger"
			PWSTR localAppData = NULL;
			HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &localAppData);
			if (SUCCEEDED(hr))
			{
				DoppelgangerRootDir_ = fs::path(localAppData);
				DoppelgangerRootDir_.make_preferred();
				DoppelgangerRootDir_.append("Doppelganger");
				fs::create_directories(DoppelgangerRootDir_);
			}
			CoTaskMemFree(localAppData);
#elif defined(__APPLE__)
			// For macOS, we use "~/Library/Application Support/Doppelganger"
			sysdir_search_path_enumeration_state state = sysdir_start_search_path_enumeration(SYSDIR_DIRECTORY_APPLICATION_SUPPORT, SYSDIR_DOMAIN_MASK_USER);
			sysdir_get_next_search_path_enumeration(state, path);
			const std::string fullPathStr = expand_user(std::string(path));

			DoppelgangerRootDir_ = fs::path(fullPathStr);
			DoppelgangerRootDir_.make_preferred();
			DoppelgangerRootDir_.append("Doppelganger");
			fs::create_directories(DoppelgangerRootDir_);
#elif defined(__linux__)
			// For Linux, we use "~/.Doppelganger"
			const char *homeEnv = std::getenv("HOME");
			std::string homeStr(homeEnv ? homeEnv : "");
			if (homeStr.empty())
			{
				struct passwd *pw = getpwuid(getuid());
				const char *homeDir = pw->pw_dir;
				homeStr = std::string(homeDir);
			}

			DoppelgangerRootDir_ = fs::path(homeStr);
			DoppelgangerRootDir_.make_preferred();
			DoppelgangerRootDir_.append(".Doppelganger");
			fs::create_directories(DoppelgangerRootDir_);
#endif
			config["DoppelgangerRootDir"] = DoppelgangerRootDir_.string();
		}

		// default config
		{
			// active
			config["active"] = true;
			// force reload
			config["forceReload"] = false;
			// browser
			config["browser"] = nlohmann::json::object();
			config.at("browser")["type"] = "default";
			config.at("browser")["openMode"] = "default";
			config.at("browser")["openOnStartup"] = true;
			// log
			config["log"] = nlohmann::json::object();
			config.at("log")["level"] = nlohmann::json::object();
			config.at("log").at("level")["SYSTEM"] = true;
			config.at("log").at("level")["APICALL"] = true;
			config.at("log").at("level")["WSCALL"] = true;
			config.at("log").at("level")["ERROR"] = true;
			config.at("log").at("level")["MISC"] = true;
			config.at("log").at("level")["DEBUG"] = true;
			config.at("log")["type"] = nlohmann::json::object();
			config.at("log").at("type")["STDOUT"] = true;
			config.at("log").at("type")["FILE"] = true;
			// output
			config["output"] = nlohmann::json::object();
			config.at("output")["type"] = "storage";
			// plugin
			config["plugin"] = nlohmann::json::object();
			config.at("plugin")["reInstall"] = true;
			config.at("plugin")["installed"] = nlohmann::json::array();
			config.at("plugin")["listURL"] = nlohmann::json::array();
			config.at("plugin").at("listURL").push_back(std::string("https://github.com/n-taka/Doppelganger_Plugin/releases/download/pluginList/pluginList_Essential.json"));
			config.at("plugin").at("listURL").push_back(std::string("https://github.com/n-taka/Doppelganger_Plugin/releases/download/pluginList/pluginList_Basic.json"));
			// server
			config["server"] = nlohmann::json::object();
			config.at("server")["certificate"] = nlohmann::json::object();
			config.at("server").at("certificate")["certificateFilePath"] = "";
			config.at("server").at("certificate")["privateKeyFilePath"] = "";
			config.at("server")["protocol"] = "http";
			config.at("server")["host"] = "127.0.0.1";
			config.at("server")["port"] = 0;
			// extension
			config["extension"] = nlohmann::json::object();
		}

		// if config.json exist, we load it
		fs::path configPath(config.at("DoppelgangerRootDir").get<std::string>());
		configPath.append("config.json");
		if (fs::exists(configPath))
		{
			std::ifstream ifs(configPath.string());
			config.merge_patch(nlohmann::json::parse(ifs));
			ifs.close();
		}

		// apply
		applyCurrentConfig();
	}

	void Core::run()
	{
		std::string completeURL("");
		{
			completeURL += config.at("server").at("protocol").get<std::string>();
			completeURL += "://";
			completeURL += config.at("server").at("host").get<std::string>();
			completeURL += ":";
			completeURL += std::to_string(config.at("server").at("portUsed").get<int>());
		}

		{
			listener_->run();
			{
				std::stringstream ss;
				ss << "Listening for requests at : " << completeURL;
				Util::log(ss.str(), "SYSTEM", config);
			}
		}
		////
		// open browser
		// "openOnStartup" = true|false
		// "browser" = "chrome"|"firefox"|"edge"|"safari"|"default"|"N/A"
		// "browserPath" = path/to/executables
		// "openMode" = "app"|"window"|"tab"|"default"
		// "cmd" = "command to be executed. use response["cmd"]+URL"
		{
			// by default, we open browser
			if (config.at("browser").at("openOnStartup").get<bool>() &&
				config.at("server").at("host").get<std::string>() == "127.0.0.1")
			{
				if (config.at("browser").at("type").get<std::string>() == "chrome")
				{
#if defined(_WIN64)
					std::vector<fs::path> chromePaths({fs::path("C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe"),
													   fs::path("C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe")});
#elif defined(__APPLE__)
					std::vector<fs::path> chromePaths({fs::path("/Applications/Google Chrome.app/Contents/MacOS/Google Chrome")});
#elif defined(__linux__)
					std::vector<fs::path> chromePaths({fs::path("/opt/google/chrome/google-chrome")});
#endif
					for (auto &p : chromePaths)
					{
						p.make_preferred();
						if (fs::exists(p))
						{
							config.at("browser")["path"] = p.string();
							break;
						}
					}
				}
				else if (config.at("browser").at("type").get<std::string>() == "firefox")
				{
#if defined(_WIN64)
					std::vector<fs::path> firefoxPaths({fs::path("C:\\Program Files\\Mozilla Firefox\\firefox.exe"),
														fs::path("C:\\Program Files (x86)\\Mozilla Firefox\\firefox.exe")});
#elif defined(__APPLE__)
					std::vector<fs::path> firefoxPaths({fs::path("/Applications/Firefox.app/Contents/MacOS/firefox")});
#elif defined(__linux__)
					std::vector<fs::path> firefoxPaths({fs::path("/usr/bin/firefox")});
#endif
					for (auto &p : firefoxPaths)
					{
						p.make_preferred();
						if (fs::exists(p))
						{
							config.at("browser")["path"] = p.string();
							break;
						}
					}
				}
				else if (config.at("browser").at("type").get<std::string>() == "edge")
				{
					// only for windows
					fs::path edgePath("C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe");
					edgePath.make_preferred();
					config.at("browser")["path"] = edgePath.string();
				}
				else if (config.at("browser").at("type").get<std::string>() == "safari")
				{
					config.at("browser")["path"] = std::string("/Applications/Safari.app");
				}
				else if (config.at("browser").at("type").get<std::string>() == "default")
				{
					// do nothing
					config.at("browser")["path"] = std::string("");
				}
				else
				{
					// N/A
					//   do nothing
				}

				// update config_.at("browser").at("openMode")
				if (config.at("browser").at("type").get<std::string>() == "chrome")
				{
					// chrome supports "app"|"window"|"tab"|"default"
					//   precisely speaking, "tab" and "default" are the same
				}
				else if (config.at("browser").at("type").get<std::string>() == "firefox")
				{
					// firefox supports "window"|"tab"|"default"
					//   precisely speaking, "tab" and "default" are the same
					if (config.at("browser").at("openMode") == "app")
					{
						config.at("browser").at("openMode") = std::string("window");
					}
				}
				else if (config.at("browser").at("type").get<std::string>() == "edge")
				{
					// edge supports "app"|"window"|"tab"|"default"
					//   precisely speaking, "tab" and "default" are the same
				}
				else if (config.at("browser").at("type").get<std::string>() == "safari")
				{
					// todo check which mode is supported by safari ...
					if (config.at("browser").at("openMode") == "app" ||
						config.at("browser").at("openMode") == "window")
					{
						config.at("browser").at("openMode") = std::string("tab");
					}
					// safari supports "tab"|"default"
					//   precisely speaking, "tab" and "default" are the same
				}
				else if (config.at("browser").at("type").get<std::string>() == "default")
				{
					// for default browser (start/open command), we cannot know which type is supported ...
					config.at("browser").at("openMode") = std::string("default");
				}
				else
				{
					// for N/A (specified by path), we cannot know which which type is supported ...
					config.at("browser").at("openMode") = std::string("default");
				}

				////
				// prepare cmd
				std::stringstream cmd;
				// browser path
#if defined(_WIN64)
				// make sure that program runs in background.
				cmd << "start ";
				cmd << "\"\" ";
				if (config.at("browser").at("type").get<std::string>() != "default")
				{
					cmd << "\"";
					cmd << config.at("browser").at("path").get<std::string>();
					cmd << "\"";
					cmd << " ";
				}
#elif defined(__APPLE__)
				if (config.at("browser").at("type").get<std::string>() == "default")
				{
					cmd << "open ";
				}
				else if (config.at("browser").at("type").get<std::string>() == "safari")
				{
					cmd << "open -a Safari.app ";
				}
				else
				{
					cmd << "\"";
					cmd << config.at("browser").at("path").get<std::string>();
					cmd << "\"";
					cmd << " ";
				}
#elif defined(__linux__)
				if (config.at("browser").at("type").get<std::string>() == "default")
				{
					cmd << "xdg-open ";
				}
				else
				{
					cmd << "\"";
					cmd << config.at("browser").at("path").get<std::string>();
					cmd << "\"";
					cmd << " ";
				}
#endif
				if (config.at("browser").at("openMode").get<std::string>() == "app")
				{
					cmd << "--app=";
				}
				else if (config.at("browser").at("openMode").get<std::string>() == "window")
				{
					cmd << "--new-window ";
				}
				else if (config.at("browser").at("openMode").get<std::string>() == "tab")
				{
					// no command line switch
				}
				else // default
				{
					// no command line switch
				}
				// URL
				cmd << completeURL;
#if defined(__APPLE__)
				if (config.at("browser").at("type").get<std::string>() != "default" &&
					config.at("browser").at("type").get<std::string>() != "safari")
				{
					cmd << " &";
				}
#elif defined(__linux__)
				if (config.at("browser").at("type").get<std::string>() != "default")
				{
					cmd << " &";
				}
#endif
				Util::log(cmd.str(), "SYSTEM", config);
				system(cmd.str().c_str());
			}
		}
	}

	void Core::applyCurrentConfig()
	{
		// DoppelgangerRootDir is ignored
		//   note: DoppelgangerRootDir is automatically specified depending on the type of OS

		// dataDir: Doppelganger/data/YYYYMMDDTHHMMSS-Core/
		//   note: dataDir is NOT changed.
		if (!config.contains("dataDir"))
		{
			fs::path dataDir_(config.at("DoppelgangerRootDir").get<std::string>());
			dataDir_.append("data");
			std::string dirName("");
			dirName += Util::getCurrentTimestampAsString(false);
			dirName += "-Core";
			dataDir_.append(dirName);
			config["dataDir"] = dataDir_.string();
			fs::create_directories(dataDir_);
		}

		// log: Doppelganger/data/YYYYMMDDTHHMMSS-Core/log
		{
			fs::path logDir(config.at("dataDir").get<std::string>());
			logDir.append("log");
			fs::create_directories(logDir);
		}

		// output: Doppelganger/data/YYYYMMDDTHHMMSS-Core/output
		{
			fs::path outputDir(config.at("dataDir").get<std::string>());
			outputDir.append("output");
			fs::create_directories(outputDir);
		}

		// plugin: Doppelganger/plugin
		//     note: actual installation is called in rooms
		{
			fs::path pluginDir(config.at("DoppelgangerRootDir").get<std::string>());
			pluginDir.append("plugin");
			fs::create_directories(pluginDir);
		}
		if (config.contains("plugin"))
		{
			if (config.at("plugin").contains("reInstall") && config.at("plugin").at("reInstall").get<bool>())
			{
				config.at("plugin").at("reInstall") = false;
				if (config.at("plugin").contains("listURL"))
				{
					// get plugin catalogue
					fs::path pluginDir(config.at("DoppelgangerRootDir").get<std::string>());
					pluginDir.append("plugin");
					nlohmann::json catalogue;
					Util::getPluginCatalogue(pluginDir, config.at("plugin").at("listURL"), catalogue);

					// For Core, we only maintain installedPlugin_
					// i.e. we *don't* perform install in Core
					std::unordered_map<std::string, nlohmann::json> plugins;

					// initialize Doppelganger::Plugin instances
					for (const auto &pluginEntry : catalogue)
					{
						const std::string name = pluginEntry.at("name").get<std::string>();
						plugins[name] = pluginEntry;
					}

					// mark installed plugins as "installed"
					{
						for (const auto &installedPlugin : config.at("plugin").at("installed"))
						{
							const std::string name = installedPlugin.at("name").get<std::string>();
							const std::string version = installedPlugin.at("version").get<std::string>();

							if (plugins.find(name) != plugins.end() && version.length() > 0)
							{
								// here we don't check validity/availability of the specified version...
								plugins.at(name)["installedVersion"] = version;
							}
							else
							{
								std::stringstream ss;
								ss << "Plugin \"" << name << "\" (" << version << ")"
								   << " is NOT found in the catalogue.";
								Util::log(ss.str(), "ERROR", config);
							}
						}
					}

					// add non-optional plugins to installedPlugin
					for (const auto &name_plugin : plugins)
					{
						const std::string &name = name_plugin.first;
						const nlohmann::json &plugin = name_plugin.second;
						if (!plugin.at("optional").get<bool>() &&
							(!plugin.contains("installedVersion")))
						{
							nlohmann::json installedPlugin = nlohmann::json::object();
							installedPlugin["name"] = name;
							installedPlugin["version"] = std::string("latest");
							config.at("plugin").at("installed").push_back(installedPlugin);
						}
					}
				}
			}
		}

		// for changing server configuration, we require reboot
		if (config.contains("server") && !listener_)
		{
			boost::system::error_code ec;

			boost::asio::ip::tcp::resolver resolver(ioc_);
			boost::asio::ip::tcp::resolver::results_type endpointIterator = resolver.resolve(
				config.at("server").at("host").get<std::string>(),
				std::to_string(config.at("server").at("port").get<int>()),
				ec);
			if (ec)
			{
				{
					std::stringstream ss;
					ss << "Fail to resolve hostname \"" << config.at("server").at("host").get<std::string>() << "\"";
					Util::log(ss.str(), "ERROR", config);
				}
				{
					std::stringstream ss;
					ss << ec.message();
					Util::log(ss.str(), "ERROR", config);
				}
				return;
			}

			boost::asio::ip::tcp::endpoint endpoint = endpointIterator->endpoint();

			if (config.at("server").at("protocol").get<std::string>() == "https")
			{
				fs::path certificateFilePath(config.at("server").at("certificate").at("certificateFilePath").get<std::string>());
				fs::path privateKeyFilePath(config.at("server").at("certificate").at("privateKeyFilePath").get<std::string>());
				if (fs::exists(certificateFilePath) && fs::exists(privateKeyFilePath))
				{
					loadServerCertificate(
						certificateFilePath,
						privateKeyFilePath);
				}
				else
				{
					{
						std::stringstream ss;
						ss << "Protocol \"https\" is specified, but no valid certificate is found. We switch to \"http\".";
						Util::log(ss.str(), "ERROR", config);
					}
					{
						std::stringstream ss;
						ss << "    certificate: ";
						ss << certificateFilePath.string();
						Util::log(ss.str(), "ERROR", config);
					}
					{
						std::stringstream ss;
						ss << "    privateKey: ";
						ss << privateKeyFilePath.string();
						Util::log(ss.str(), "ERROR", config);
					}
					config.at("server").at("protocol") = std::string("http");
				}
			}

			// listener
			listener_ = std::make_shared<Listener>(weak_from_this(), ioc_, ctx_, endpoint);
			config.at("server")["portUsed"] = listener_->acceptor_.local_endpoint().port();
		}

		// "active" and "forceReload" are very critical and we take care of them in the last
		if (config.contains("active") && !config.at("active").get<bool>())
		{
			// shutdown
			storeCurrentConfig();
			ioc_.stop();
			// todo: close rooms, etc.
			return;
		}

		if (config.contains("forceReload") && config.at("forceReload").get<bool>())
		{
			config.at("forceReload") = false;
			for (const auto &uuid_room : rooms_)
			{
				const std::shared_ptr<Room> room = uuid_room.second;
				// reload
				room->broadcastWS(std::string("forceReload"), std::string(""), nlohmann::json::object(), nlohmann::basic_json(nullptr));
			}
		}
	}

	void Core::storeCurrentConfig() const
	{
		fs::path configPath(config.at("DoppelgangerRootDir").get<std::string>());
		configPath.append("config.json");
		std::ofstream ofs(configPath.string());
		if (ofs)
		{
			nlohmann::json configToBeStored = config;
			// remove unused
			configToBeStored.erase("active");
			configToBeStored.erase("forceReload");
			configToBeStored.erase("DoppelgangerRootDir");
			configToBeStored.erase("dataDir");
			configToBeStored.at("browser").erase("path");
			configToBeStored.at("plugin").erase("reInstall");
			configToBeStored.at("server").erase("portUsed");

			ofs << configToBeStored.dump(4, ' ', true);
		}
		ofs.close();
	}

	void Core::loadServerCertificate(const fs::path &certificatePath, const fs::path &privateKeyPath)
	{

		const std::string dh =
			"-----BEGIN DH PARAMETERS-----\n"
			"MIIBCAKCAQEAiPqG0guWzJK/5BENNAhWoxVDNUjEc7FtkVxtwjXYrUUcvL2Db5ni\n"
			"MlhPqsEtY8n7n0vTcZjvpHuyVgChSXIrw17OYrtjDONz0MtZygKVN99DNVzmhsvX\n"
			"Km7J55r48ktqSzNwJSxRe3GE1Kv3Q8P3KwDHlmDfKXMKs29gUFElus5J9n7YFzJ6\n"
			"aNOCjxJRSMKM9P0pdSE/acQTQzlMHEk/9ndJfINxLcagt8OMlqroFl3mOvSpIV0a\n"
			"EJ/jJZlH+LEUqrPX7ZnnSeHOH2AOODagMPf07iGWIuEhS4IcY3AmD5V1HZqAg/y2\n"
			"JoiGl/8+RNpL1HlmZXwNXVv4zUIHirskCwIBAg==\n"
			"-----END DH PARAMETERS-----";

		ctx_.set_password_callback(
			[](std::size_t,
			   boost::asio::ssl::context_base::password_purpose)
			{
				return "test";
			});

		// we only support TLS
		// we always generate dh params every time
		ctx_.set_options(
			boost::asio::ssl::context::default_workarounds |
			boost::asio::ssl::context::no_sslv2 |
			boost::asio::ssl::context::no_sslv3 |
			boost::asio::ssl::context::single_dh_use);

		ctx_.use_certificate_chain_file(certificatePath.string());

		ctx_.use_private_key_file(privateKeyPath.string(), boost::asio::ssl::context::file_format::pem);

		// we always generate dh params every time
		ctx_.use_tmp_dh(
			boost::asio::buffer(dh.data(), dh.size()));
	}

}

#endif
