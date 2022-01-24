#ifndef CORE_CPP
#define CORE_CPP

#include "Doppelganger/Core.h"
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
		// set default parameters

		// active
		active_ = true;
		// broser setting
		browserConfig_ = BrowserConfig({true,
										std::string("default"),
										std::string("default"),
										fs::path()});
		// root
		DoppelgangerRootDir_ = fs::path();
		// data
		dataDir_ = fs::path();
		// log
		logConfig_.level["SYSTEM"] = true;
		logConfig_.level["APICALL"] = true;
		logConfig_.level["WSCALL"] = true;
		logConfig_.level["ERROR"] = true;
		logConfig_.level["MISC"] = true;
		logConfig_.level["DEBUG"] = true;
		logConfig_.type["STDOUT"] = true;
		logConfig_.type["FILE"] = true;
		// output
		outputType_ = std::string("storage");
		// plugin
		plugin_ = std::unordered_map<std::string, Doppelganger::Plugin>();
		// installed plugin
		installedPlugin_ = std::vector<PluginInfo>();
		// pluginList
		pluginListURL_.push_back(std::string("https://n-taka.info/nextcloud/s/XqGGYPo8J2rwc9S/download/pluginList_Essential.json"));
		pluginListURL_.push_back(std::string("https://n-taka.info/nextcloud/s/PgccNTmPECPXSgQ/download/pluginList_Basic.json"));
		pluginListURL_.push_back(std::string("https://n-taka.info/nextcloud/s/PWPR7YDXKoMeP66/download/pluginList_TORIDE.json"));
		// server
		serverConfig_ = ServerConfig({std::string("127.0.0.1"),
									  0,
									  -1,
									  std::string("http"),
									  fs::path(),
									  fs::path()});
	}

	void Core::setup()
	{
		////
		// path for DoppelgangerRoot
		{
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
		}

		{
			fs::path configPath(DoppelgangerRootDir_);
			configPath.append("config.json");
			if (fs::exists(configPath))
			{
				// we could directly use std::filesystem::path, but we could not directly boost::filesystem::path
				std::ifstream ifs(configPath.string());
				from_json(nlohmann::json::parse(ifs));
				ifs.close();
			}
			// default config is already stored in the constructor
		}

		// apply current config
		applyCurrentConfig();
	}

	void Core::run()
	{
		std::string completeURL("");
		{
			completeURL += serverConfig_.protocol;
			completeURL += "://";
			completeURL += serverConfig_.host;
			completeURL += ":";
			completeURL += std::to_string(serverConfig_.portUsed);
		}

		{
			listener_->run();

			{
				std::stringstream ss;
				ss << "Listening for requests at : " << completeURL;
				Util::log(ss.str(), "SYSTEM", dataDir_, logConfig_.level, logConfig_.type);
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
			if (browserConfig_.openOnStartup &&
				serverConfig_.host == "127.0.0.1")
			{
				// update config_.at("browser").at("path")
				if (browserConfig_.type == "chrome")
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
							browserConfig_.path = p.string();
							break;
						}
					}
				}
				else if (browserConfig_.type == "firefox")
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
							browserConfig_.path = p.string();
							break;
						}
					}
				}
				else if (browserConfig_.type == "edge")
				{
					// only for windows
					fs::path edgePath("C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe");
					edgePath.make_preferred();
					browserConfig_.path = edgePath.string();
				}
				else if (browserConfig_.type == "safari")
				{
					browserConfig_.path = std::string("/Applications/Safari.app");
				}
				else if (browserConfig_.type == "default")
				{
					// do nothing
					browserConfig_.path = std::string("");
				}
				else
				{
					// N/A
					//   do nothing
				}

				// update config_.at("browser").at("openMode")
				if (browserConfig_.type == "chrome")
				{
					// chrome supports "app"|"window"|"tab"|"default"
					//   precisely speaking, "tab" and "default" are the same
				}
				else if (browserConfig_.type == "firefox")
				{
					// firefox supports "window"|"tab"|"default"
					//   precisely speaking, "tab" and "default" are the same
					if (browserConfig_.openMode == "app")
					{
						browserConfig_.openMode = std::string("window");
					}
				}
				else if (browserConfig_.type == "edge")
				{
					// edge supports "app"|"window"|"tab"|"default"
					//   precisely speaking, "tab" and "default" are the same
				}
				else if (browserConfig_.type == "safari")
				{
					// todo check which mode is supported by safari ...
					if (browserConfig_.openMode == "app" ||
						browserConfig_.openMode == "window")
					{
						browserConfig_.openMode = std::string("tab");
					}
					// safari supports "tab"|"default"
					//   precisely speaking, "tab" and "default" are the same
				}
				else if (browserConfig_.type == "default")
				{
					// for default browser (start/open command), we cannot know which type is supported ...
					browserConfig_.openMode = std::string("default");
				}
				else
				{
					// for N/A (specified by path), we cannot know which which type is supported ...
					browserConfig_.openMode = std::string("default");
				}

				////
				// prepare cmd
				std::stringstream cmd;
				// browser path
#if defined(_WIN64)
				// make sure that program runs in background.
				cmd << "start ";
				cmd << "\"\" ";
				if (browserConfig_.type != "default")
				{
					cmd << "\"";
					cmd << browserConfig_.path;
					cmd << "\"";
					cmd << " ";
				}
#elif defined(__APPLE__)
				if (browserConfig_.type == "default")
				{
					cmd << "open ";
				}
				else if (browserConfig_.type == "safari")
				{
					cmd << "open -a Safari.app ";
				}
				else
				{
					cmd << "\"";
					cmd << browserConfig_.path;
					cmd << "\"";
					cmd << " ";
				}
#elif defined(__linux__)
				if (browserConfig_.type == "default")
				{
					cmd << "xdg-open ";
				}
				else
				{
					cmd << "\"";
					cmd << browserConfig_.path;
					cmd << "\"";
					cmd << " ";
				}
#endif
				if (browserConfig_.openMode == "app")
				{
					cmd << "--app=";
				}
				else if (browserConfig_.openMode == "window")
				{
					cmd << "--new-window ";
				}
				else if (browserConfig_.openMode == "tab")
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
				if (browserConfig_.type != "default" && browserConfig_.type != "safari")
				{
					cmd << " &";
				}
#elif defined(__linux__)
				if (browserConfig_.type != "default")
				{
					cmd << " &";
				}
#endif
				Util::log(cmd.str(), "SYSTEM", dataDir_, logConfig_.level, logConfig_.type);
				system(cmd.str().c_str());
			}
		}
	}

	void Core::to_json(nlohmann::json &json) const
	{
		json = nlohmann::json::object();
		json["active"] = active_;
		json["browser"] = nlohmann::json::object();
		{
			json.at("browser")["openOnStartup"] = browserConfig_.openOnStartup;
			json.at("browser")["openMode"] = browserConfig_.openMode;
			json.at("browser")["type"] = browserConfig_.type;
			// path is not stored in json
		}
		json["DoppelgangerRootDir"] = DoppelgangerRootDir_.string();
		json["log"] = nlohmann::json::object();
		{
			json.at("log")["level"] = nlohmann::json::object();
			for (const auto &level_value : logConfig_.level)
			{
				const std::string &level = level_value.first;
				const bool &value = level_value.second;
				json.at("log").at("level")[level] = value;
			}
			json.at("log")["type"] = nlohmann::json::object();
			for (const auto &type_value : logConfig_.type)
			{
				const std::string &type = type_value.first;
				const bool &value = type_value.second;
				json.at("log").at("type")[type] = value;
			}
		}
		json["output"] = nlohmann::json::object();
		{
			json.at("output")["type"] = outputType_;
		}
		json["plugin"] = nlohmann::json::object();
		{
			json.at("plugin")["installed"] = nlohmann::json::array();
			for (const auto &plugin : installedPlugin_)
			{
				nlohmann::json pluginJson = nlohmann::json::object();
				pluginJson["name"] = plugin.name;
				pluginJson["version"] = plugin.version;
				json.at("plugin").at("installed").push_back(pluginJson);
			}
			json.at("plugin")["listURL"] = nlohmann::json::array();
			for (const auto &listURL : pluginListURL_)
			{
				json.at("plugin").at("listURL").push_back(listURL);
			}
		}
		json["server"] = nlohmann::json::object();
		{
			json.at("server")["host"] = serverConfig_.host;
			json.at("server")["port"] = serverConfig_.portRequested;
			// portUsed is not stored in json
			json.at("server")["protocol"] = serverConfig_.protocol;
			json.at("server")["certificate"] = nlohmann::json::object();
			json.at("server").at("certificate")["certificateFilePath"] = serverConfig_.certificateFilePath.string();
			json.at("server").at("certificate")["privateKeyFilePath"] = serverConfig_.privateKeyFilePath.string();
		}
		json["extension"] = extension_;
	}

	void Core::from_json(const nlohmann::json &json)
	{
		active_ = json.at("active").get<bool>();

		browserConfig_.openOnStartup = json.at("browser").at("openOnStartup").get<bool>();
		browserConfig_.openMode = json.at("browser").at("openMode").get<std::string>();
		browserConfig_.type = json.at("browser").at("type").get<std::string>();
		// DoppelgangerRootDir is ignored
		logConfig_.level.clear();
		for (const auto &level_value : json.at("log").at("level").items())
		{
			const std::string &level = level_value.key();
			const bool &value = level_value.value().get<bool>();
			logConfig_.level[level] = value;
		}
		logConfig_.type.clear();
		for (const auto &type_value : json.at("log").at("type").items())
		{
			const std::string &type = type_value.key();
			const bool &value = type_value.value().get<bool>();
			logConfig_.type[type] = value;
		}

		outputType_ = json.at("output").at("type").get<std::string>();

		installedPlugin_.clear();
		for (const auto &plugin : json.at("plugin").at("installed"))
		{
			PluginInfo pluginInfo;
			pluginInfo.name = plugin.at("name").get<std::string>();
			pluginInfo.version = plugin.at("version").get<std::string>();
			installedPlugin_.push_back(pluginInfo);
		}

		pluginListURL_.clear();
		for (const auto &listURL : json.at("plugin").at("listURL"))
		{
			pluginListURL_.push_back(listURL.get<std::string>());
		}

		serverConfig_.host = json.at("server").at("host").get<std::string>();
		serverConfig_.portRequested = json.at("server").at("port").get<int>();
		serverConfig_.protocol = json.at("server").at("protocol").get<std::string>();
		if (json.at("server").contains("certificate"))
		{
			serverConfig_.certificateFilePath = fs::path(json.at("server").at("certificate").at("certificateFilePath").get<std::string>());
			serverConfig_.privateKeyFilePath = fs::path(json.at("server").at("certificate").at("privateKeyFilePath").get<std::string>());
		}

		extension_ = json.at("extension");
	}

	void Core::applyCurrentConfig()
	{
		if (!active_)
		{
			// shutdown...
			storeCurrentConfig();
			ioc_.stop();
			// todo: close rooms, etc.
			return;
		}

		// dataDir: Doppelganger/data/YYYYMMDDTHHMMSS-Core/
		//   note: dataDir is NOT changed.
		if (!fs::exists(dataDir_))
		{
			dataDir_ = DoppelgangerRootDir_;
			dataDir_.append("data");
			std::string dirName("");
			dirName += Util::getCurrentTimestampAsString(false);
			dirName += "-Core";
			dataDir_.append(dirName);
			fs::create_directories(dataDir_);
		}

		// log: Doppelganger/data/YYYYMMDDTHHMMSS-Core/log
		{
			fs::path logDir(dataDir_);
			logDir.append("log");
			fs::create_directories(logDir);
		}

		// output: Doppelganger/data/YYYYMMDDTHHMMSS-Core/output
		{
			fs::path outputDir(dataDir_);
			outputDir.append("output");
			fs::create_directories(outputDir);
		}

		// plugin: Doppelganger/plugin
		//     note: actual installation is called in rooms
		{
			fs::path pluginDir(DoppelgangerRootDir_);
			pluginDir.append("plugin");
			fs::create_directories(pluginDir);
		}

		// plugin
		{
			// remove outdated ones
			plugin_.clear();

			// get plugin catalogue
			fs::path pluginDir(DoppelgangerRootDir_);
			pluginDir.append("plugin");
			nlohmann::json catalogue;
			Util::getPluginCatalogue(pluginDir, pluginListURL_, catalogue);

			// initialize Doppelganger::Plugin instances
			for (const auto &pluginEntry : catalogue)
			{
				Doppelganger::Plugin plugin = pluginEntry.get<Doppelganger::Plugin>();
				plugin_[plugin.name_] = plugin;
			}

			// install plugins
			{
				for (const auto &plugin : installedPlugin_)
				{
					const std::string &name = plugin.name;
					const std::string &version = plugin.version;

					if (plugin_.find(name) != plugin_.end() && version.size() > 0)
					{
						plugin_.at(name).install(weak_from_this(), version);
					}
					else
					{
						std::stringstream ss;
						ss << "Plugin \"" << name << "\" (" << version << ")"
						   << " is NOT found.";
						Util::log(ss.str(), "ERROR", dataDir_, logConfig_.level, logConfig_.type);
					}
				}
			}

			// install non-optional plugins
			for (auto &name_plugin : plugin_)
			{
				Plugin &plugin = name_plugin.second;
				if (!plugin.optional_ && (plugin.installedVersion_.size() == 0))
				{
					plugin.install(weak_from_this(), std::string("latest"));
				}
			}
		}

		// server
		{
			boost::system::error_code ec;

			boost::asio::ip::tcp::resolver resolver(ioc_);
			boost::asio::ip::tcp::resolver::results_type endpointIterator = resolver.resolve(
				serverConfig_.host,
				std::to_string(serverConfig_.portRequested),
				ec);
			if (ec)
			{
				{
					std::stringstream ss;
					ss << "Fail to resolve hostname \"" << serverConfig_.host << "\"";
					Util::log(ss.str(), "ERROR", dataDir_, logConfig_.level, logConfig_.type);
				}
				{
					std::stringstream ss;
					ss << ec.message();
					Util::log(ss.str(), "ERROR", dataDir_, logConfig_.level, logConfig_.type);
				}
				return;
			}

			boost::asio::ip::tcp::endpoint endpoint = endpointIterator->endpoint();

			if (serverConfig_.protocol == "https")
			{
				if (fs::exists(serverConfig_.certificateFilePath) && fs::exists(serverConfig_.privateKeyFilePath))
				{
					loadServerCertificate(
						serverConfig_.certificateFilePath,
						serverConfig_.privateKeyFilePath);
				}
				else
				{
					{
						std::stringstream ss;
						ss << "Protocol \"https\" is specified, but no valid certificate is found. We switch to \"http\".";
						Util::log(ss.str(), "ERROR", dataDir_, logConfig_.level, logConfig_.type);
					}
					{
						std::stringstream ss;
						ss << "    certificate: ";
						ss << serverConfig_.certificateFilePath.string();
						Util::log(ss.str(), "ERROR", dataDir_, logConfig_.level, logConfig_.type);
					}
					{
						std::stringstream ss;
						ss << "    privateKey: ";
						ss << serverConfig_.privateKeyFilePath.string();
						Util::log(ss.str(), "ERROR", dataDir_, logConfig_.level, logConfig_.type);
					}
					serverConfig_.protocol = std::string("http");
				}
			}

			// listener
			listener_ = std::make_shared<Listener>(weak_from_this(), ioc_, ctx_, endpoint);
			serverConfig_.portUsed = listener_->acceptor_.local_endpoint().port();
		}
	}

	void Core::storeCurrentConfig() const
	{
		nlohmann::json config;
		to_json(config);

		fs::path configPath(DoppelgangerRootDir_);
		configPath.append("config.json");
		std::ofstream ofs(configPath.string());
		if (ofs)
		{
			ofs << config.dump(4);
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
