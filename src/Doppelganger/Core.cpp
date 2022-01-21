#ifndef CORE_CPP
#define CORE_CPP

#include "Doppelganger/Core.h"
#include "Doppelganger/HTTPSession.h"
#include "Doppelganger/Plugin.h"
#include "Doppelganger/Listener.h"
#include "Doppelganger/Util/download.h"
#include "Doppelganger/Util/getCurrentTimestampAsString.h"
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
		////
		// path for DoppelgangerRoot
		fs::path DoppelgangerRootDir;
		{
#if defined(_WIN64)
			// For windows, we use "%USERPROFILE%\AppData\Local\Doppelganger"
			// e.g. "C:\Users\KazutakaNakashima\AppData\Local\Doppelganger"
			PWSTR localAppData = NULL;
			HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &localAppData);
			if (SUCCEEDED(hr))
			{
				DoppelgangerRootDir = fs::path(localAppData);
				DoppelgangerRootDir.make_preferred();
				DoppelgangerRootDir.append("Doppelganger");
				fs::create_directories(DoppelgangerRootDir);
			}
			CoTaskMemFree(localAppData);
#elif defined(__APPLE__)
			// For macOS, we use "~/Library/Application Support/Doppelganger"
			sysdir_search_path_enumeration_state state = sysdir_start_search_path_enumeration(SYSDIR_DIRECTORY_APPLICATION_SUPPORT, SYSDIR_DOMAIN_MASK_USER);
			sysdir_get_next_search_path_enumeration(state, path);
			const std::string fullPathStr = expand_user(std::string(path));

			DoppelgangerRootDir = fs::path(fullPathStr);
			DoppelgangerRootDir.make_preferred();
			DoppelgangerRootDir.append("Doppelganger");
			fs::create_directories(DoppelgangerRootDir);
#elif defined(__linux__)
			// For macOS, we use "~/.Doppelganger"
			const char *homeEnv = std::getenv("HOME");
			std::string homeStr(homeEnv ? homeEnv : "");
			if (homeStr.empty())
			{
				struct passwd *pw = getpwuid(getuid());
				const char *homeDir = pw->pw_dir;
				homeStr = std::string(homeDir);
			}

			DoppelgangerRootDir = fs::path(homeStr);
			DoppelgangerRootDir.make_preferred();
			DoppelgangerRootDir.append(".Doppelganger");
			fs::create_directories(DoppelgangerRootDir);
#endif
		}

		{
			fs::path configPath(DoppelgangerRootDir);
			configPath.append("config.json");
			if (fs::exists(configPath))
			{
				// we could directly use std::filesystem::path, but we could not directly boost::filesystem::path
				std::ifstream ifs(configPath.string());
				config_ = nlohmann::json::parse(ifs);
				ifs.close();
			}
			else
			{
				// no config file is found. (e.g. use Doppelganger first time)
				generateDefaultConfigJson(config_);
				std::cout << "No config file found. We use default config." << std::endl;
			}
			// store path to root dir
			config_["DoppelgangerRootDir"] = DoppelgangerRootDir.string();
		}

		// apply current config
		applyCurrentConfig();
	}

	void Core::run()
	{
		{
			listener_->run();

			{
				std::stringstream ss;
				ss << "Listening for requests at : " << config_.at("server").at("completeURL").get<std::string>();
				Util::log(ss.str(), "SYSTEM", config_);
			}
		}
		////
		// open browser
		// "openOnStartup" = true|false
		// "browser" = "chrome"|"firefox"|"edge"|"safari"|"default"|"N/A"
		// "browserPath" = path/to/executables
		// "openAs" = "app"|"window"|"tab"|"default"
		// "cmd" = "command to be executed. use response["cmd"]+URL"
		{
			// by default, we open browser
			if (config_.at("browser").at("openOnStartup").get<bool>() &&
				config_.at("server").at("host").get<std::string>() == "127.0.0.1")
			{
				// update config_.at("browser").at("path")
				if (config_.at("browser").at("type").get<std::string>() == "chrome")
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
							config_.at("browser").at("path") = p.string();
							break;
						}
					}
				}
				else if (config_.at("browser").at("type").get<std::string>() == "firefox")
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
							config_.at("browser").at("path") = p.string();
							break;
						}
					}
				}
				else if (config_.at("browser").at("type").get<std::string>() == "edge")
				{
					// only for windows
					fs::path edgePath("C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe");
					edgePath.make_preferred();
					config_.at("browser").at("path") = edgePath.string();
				}
				else if (config_.at("browser").at("type").get<std::string>() == "safari")
				{
					config_.at("browser").at("path") = std::string("/Applications/Safari.app");
				}
				else if (config_.at("browser").at("type").get<std::string>() == "default")
				{
					// do nothing
					config_.at("browser").at("path") = std::string("");
				}
				else
				{
					// N/A
					//   do nothing
				}

				// update config_.at("browser").at("openAs")
				if (config_.at("browser").at("type").get<std::string>() == "chrome")
				{
					// chrome supports "app"|"window"|"tab"|"default"
					//   precisely speaking, "tab" and "default" are the same
				}
				else if (config_.at("browser").at("type").get<std::string>() == "firefox")
				{
					// firefox supports "window"|"tab"|"default"
					//   precisely speaking, "tab" and "default" are the same
					if (config_.at("browser").at("openAs").get<std::string>() == "app")
					{
						config_.at("browser").at("openAs") = std::string("window");
					}
				}
				else if (config_.at("browser").at("type").get<std::string>() == "edge")
				{
					// edge supports "app"|"window"|"tab"|"default"
					//   precisely speaking, "tab" and "default" are the same
				}
				else if (config_.at("browser").at("type").get<std::string>() == "safari")
				{
					// todo check which mode is supported by safari ...
					if (config_.at("browser").at("openAs").get<std::string>() == "app" || config_.at("browser").at("openAs").get<std::string>() == "window")
					{
						config_.at("browser").at("openAs") = std::string("tab");
					}
					// safari supports "tab"|"default"
					//   precisely speaking, "tab" and "default" are the same
				}
				else if (config_.at("browser").at("type").get<std::string>() == "default")
				{
					// for default browser (start/open command), we cannot know which type is supported ...
					config_.at("browser").at("openAs") = std::string("default");
				}
				else
				{
					// for N/A (specified by path), we cannot know which which type is supported ...
					config_.at("browser").at("openAs") = std::string("default");
				}

				////
				// prepare cmd
				std::stringstream cmd;
				// browser path
#if defined(_WIN64)
				// make sure that program runs in background.
				cmd << "start ";
				cmd << "\"\" ";
				if (config_.at("browser").at("type").get<std::string>() != "default")
				{
					cmd << "\"";
					cmd << config_.at("browser").at("path").get<std::string>();
					cmd << "\"";
					cmd << " ";
				}
#elif defined(__APPLE__)
				if (config_.at("browser").at("type").get<std::string>() == "default")
				{
					cmd << "open ";
				}
				else if (config_.at("browser").at("type").get<std::string>() == "safari")
				{
					cmd << "open -a Safari.app ";
				}
				else
				{
					cmd << "\"";
					cmd << config_.at("browser").at("path").get<std::string>();
					cmd << "\"";
					cmd << " ";
				}
#elif defined(__linux__)
				if (config_.at("browser").at("type").get<std::string>() == "default")
				{
					cmd << "xdg-open ";
				}
				else
				{
					cmd << "\"";
					cmd << config_.at("browser").at("path").get<std::string>();
					cmd << "\"";
					cmd << " ";
				}
#endif
				if (config_.at("browser").at("openAs").get<std::string>() == "app")
				{
					cmd << "--app=";
				}
				else if (config_.at("browser").at("openAs").get<std::string>() == "window")
				{
					cmd << "--new-window ";
				}
				else if (config_.at("browser").at("openAs").get<std::string>() == "tab")
				{
					// no command line switch
				}
				else // default
				{
					// no command line switch
				}
				// URL
				cmd << config_.at("server").at("completeURL").get<std::string>();
#if defined(__APPLE__)
				if (config_.at("browser").at("type").get<std::string>() != "default" && config_.at("browser").at("type").get<std::string>() != "safari")
				{
					cmd << " &";
				}
#elif defined(__linux__)
				if (config_.at("browser").at("type").get<std::string>() != "default")
				{
					cmd << " &";
				}
#endif

				config_.at("browser").at("cmd") = cmd.str();
				Util::log(cmd.str(), "SYSTEM", config_);
				system(cmd.str().c_str());
			}
		}
	}

	void Core::applyCurrentConfig()
	{
		if (!config_.at("active").get<bool>())
		{
			// shutdown...
			ioc_.stop();
			// todo: close rooms, etc.
			return;
		}

		// dataDir: Doppelganger/data/YYYYMMDDTHHMMSS-Core/
		//   note: dataDir is NOT changed.
		if (!config_.at("data").at("initialized").get<bool>())
		{
			fs::path dataDir(config_.at("DoppelgangerRootDir").get<std::string>());
			dataDir.append("data");
			std::string dirName("");
			dirName += Util::getCurrentTimestampAsString(false);
			dirName += "-Core";
			dataDir.append(dirName);
			fs::create_directories(dataDir);
			config_.at("data").at("dir") = dataDir.string();
			config_.at("data").at("initialized") = true;
		}

		// log: Doppelganger/data/YYYYMMDDTHHMMSS-Core/log
		{
			fs::path logDir(config_.at("data").at("dir").get<std::string>());
			logDir.append("log");
			fs::create_directories(logDir);
		}

		// output: Doppelganger/data/YYYYMMDDTHHMMSS-Core/output
		{
			fs::path outputDir(config_.at("data").at("dir").get<std::string>());
			outputDir.append("output");
			fs::create_directories(outputDir);
		}

		// plugin: Doppelganger/plugin
		{
			fs::path pluginDir(config_.at("DoppelgangerRootDir").get<std::string>());
			pluginDir.append("plugin");
			fs::create_directories(pluginDir);
		}
		{
			// get plugin catalogue
			nlohmann::json catalogue;
			Plugin::getCatalogue(config_, catalogue);

			// initialize Doppelganger::Plugin instances
			for (const auto &pluginEntry : catalogue.items())
			{
				const std::string &name = pluginEntry.key();
				const nlohmann::json &pluginInfo = pluginEntry.value();
				plugin_[name] = std::make_unique<Doppelganger::Plugin>(name, pluginInfo);
			}

			// install plugins
			{
				const nlohmann::ordered_json installedPluginJson = config_.at("plugin").at("installed");
				std::vector<std::string> sortedInstalledPluginName(installedPluginJson.size());
				for (const auto &installedPlugin : installedPluginJson.items())
				{
					const std::string &name = installedPlugin.key();
					const nlohmann::json &value = installedPlugin.value();
					const int index = value.at("priority").get<int>();
					sortedInstalledPluginName.at(index) = name;
				}

				for (const auto &plugin : installedPluginJson.items())
				{
					const std::string &name = plugin.key();
					const nlohmann::json &info = plugin.value();
					const std::string &version = info.at("version").get<std::string>();

					if (plugin_.find(name) != plugin_.end() && version.size() > 0)
					{
						plugin_.at(name)->install(config_, version);
					}
					else
					{
						std::stringstream ss;
						ss << "Plugin \"" << name << "\" (" << version << ")"
						   << " is NOT found.";
						Util::log(ss.str(), "ERROR", config_);
					}
				}
			}

			// install non-optional plugins
			//   note: here, we install to Core, but installed plugin is NOT called...
			for (const auto &pluginEntry : catalogue.items())
			{
				const std::string &name = pluginEntry.key();
				const nlohmann::json &info = pluginEntry.value();
				const bool &optional = info.at("optional").get<bool>();
				if (!optional)
				{
					if (plugin_.find(name) != plugin_.end())
					{
						if (plugin_.at(name)->installedVersion_.size() == 0)
						{
							plugin_.at(name)->install(config_, std::string("latest"));
						}
					}
					else
					{
						std::stringstream ss;
						ss << "Non-optional plugin \"" << name << "\" (latest)"
						   << " is NOT found.";
						Util::log(ss.str(), "ERROR", config_);
					}
				}
			}
		}

		// server
		{
			boost::system::error_code ec;

			boost::asio::ip::tcp::resolver resolver(ioc_);
			boost::asio::ip::tcp::resolver::results_type endpointIterator = resolver.resolve(config_.at("server").at("host").get<std::string>(), std::to_string(config_.at("server").at("port").get<int>()), ec);
			if (ec)
			{
				{
					std::stringstream ss;
					ss << "Fail to resolve hostname \"" << config_.at("server").at("host").get<std::string>() << "\"";
					Util::log(ss.str(), "ERROR", config_);
				}
				{
					std::stringstream ss;
					ss << ec.message();
					Util::log(ss.str(), "ERROR", config_);
				}
				return;
			}

			boost::asio::ip::tcp::endpoint endpoint = endpointIterator->endpoint();

			if (config_.at("server").at("protocol").get<std::string>() == "https")
			{
				if (config_.at("server").contains("certificateFiles"))
				{
					fs::path certificatePath, privateKeyPath;
					if (config_.at("server").at("certificateFiles").contains("certificate"))
					{
						certificatePath = fs::path(config_.at("server").at("certificateFiles").at("certificate").get<std::string>());
					}
					if (config_.at("server").at("certificateFiles").contains("privateKey"))
					{
						privateKeyPath = fs::path(config_.at("server").at("certificateFiles").at("privateKey").get<std::string>());
					}
					if (fs::exists(certificatePath) && fs::exists(privateKeyPath))
					{
						loadServerCertificate(certificatePath, privateKeyPath);
					}
					else
					{
						{
							std::stringstream ss;
							ss << "Protocol \"https\" is specified, but no valid certificate is found. We switch to \"http\".";
							Util::log(ss.str(), "ERROR", config_);
						}
						{
							std::stringstream ss;
							ss << "    certificate: ";
							ss << certificatePath;
							Util::log(ss.str(), "ERROR", config_);
						}
						{
							std::stringstream ss;
							ss << "    privateKey: ";
							ss << privateKeyPath;
							Util::log(ss.str(), "ERROR", config_);
						}
						config_.at("server").at("protocol") = std::string("http");
					}
				}
				else
				{
					config_.at("server").at("protocol") = std::string("http");
				}
			}

			// listener
			listener_ = std::make_shared<Listener>(weak_from_this(), ioc_, ctx_, endpoint);
			{
				config_.at("server").at("port") = listener_->acceptor_.local_endpoint().port();

				std::string completeURL("");
				completeURL = config_.at("server").at("protocol").get<std::string>();
				completeURL += "://";
				completeURL += config_.at("server").at("host").get<std::string>();
				completeURL += ":";
				completeURL += std::to_string(config_.at("server").at("port").get<int>());
				config_.at("server").at("completeURL") = completeURL;
			}
		}
	}

	void Core::storeCurrentConfig() const
	{
		// todo
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

	void Core::generateDefaultConfigJson(nlohmann::json &config)
	{
		config = nlohmann::json::object();
		{
			config["active"] = true;
			config["browser"] = nlohmann::json::object();
			{
				config["browser"]["openAs"] = std::string("default");
				config["browser"]["openOnStartup"] = true;
				config["browser"]["path"] = std::string("");
				config["browser"]["type"] = std::string("default");
			}
			config["DoppelgangerRootDir"] = std::string("");
			config["data"] = nlohmann::json::object();
			{
				config["data"]["dir"] = std::string("");
				config["data"]["initialized"] = false;
			}
			config["log"] = nlohmann::json::object();
			{
				config["log"]["level"] = nlohmann::json::object();
				{
					config["log"]["level"]["SYSTEM"] = true;
					config["log"]["level"]["APICALL"] = true;
					config["log"]["level"]["WSCALL"] = true;
					config["log"]["level"]["ERROR"] = true;
					config["log"]["level"]["MISC"] = true;
					config["log"]["level"]["DEBUG"] = true;
				}
				config["log"]["type"] = nlohmann::json::object();
				{
					config["log"]["type"]["STDOUT"] = true;
					config["log"]["type"]["FILE"] = true;
				}
			}
			config["output"] = nlohmann::json::object();
			{
				config["output"]["type"] = std::string("storage");
			}
			config["plugin"] = nlohmann::json::object();
			{
				config["plugin"]["listURL"] = nlohmann::json::array();
				{
					config["plugin"]["listURL"].push_back(std::string("https://n-taka.info/nextcloud/s/XqGGYPo8J2rwc9S/download/pluginList_Essential.json"));
					config["plugin"]["listURL"].push_back(std::string("https://n-taka.info/nextcloud/s/PgccNTmPECPXSgQ/download/pluginList_Basic.json"));
					config["plugin"]["listURL"].push_back(std::string("https://n-taka.info/nextcloud/s/PWPR7YDXKoMeP66/download/pluginList_TORIDE.json"));
				}
				// IMPORTANT
				// we consider the order as the order of the installation
				config["plugin"]["installed"] = nlohmann::ordered_json::object();
			}
			config["server"] = nlohmann::json::object();
			{
				config["server"]["host"] = std::string("127.0.0.1");
				config["server"]["port"] = 0;
				config["server"]["protocol"] = std::string("http");
				config["server"]["certificateFiles"] = nlohmann::json::object();
				{
					config["server"]["certificateFiles"]["certificate"] = std::string("");
					config["server"]["certificateFiles"]["privateKey"] = std::string("");
				}
				config["server"]["completeURL"] = std::string("");
			}
		}
	}
}

#endif
