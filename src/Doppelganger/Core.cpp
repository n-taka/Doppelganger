#ifndef CORE_CPP
#define CORE_CPP

#include "Doppelganger/Core.h"
#include "Doppelganger/Logger.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/HTTPSession.h"
#include "Doppelganger/Plugin.h"
#include "Util/download.h"

#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

// https://github.com/boostorg/beast/blob/develop/example/http/server/async/http_server_async.cpp

namespace Doppelganger
{
	Core::Core(boost::asio::io_context &ioc_)
		: ioc(ioc_), acceptor(ioc), socket(ioc)
	{
	}

	void Core::setup()
	{
		////
		// setup for resourceDir/workingDir
		{
#if defined(_WIN32) || defined(_WIN64)
			char buffer[MAX_PATH];
			// for windows, we use the directory of .exe itself
			GetModuleFileName(NULL, buffer, sizeof(buffer));
			fs::path p(buffer);
			p = p.parent_path();
			systemParams.resourceDir = p;
			systemParams.resourceDir.make_preferred();
			systemParams.resourceDir.append("resources");
			systemParams.workingDir = systemParams.resourceDir;
#elif defined(__APPLE__)
			CFURLRef appUrlRef;
			appUrlRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFSTR("index"), CFSTR(".html"), CFSTR("html"));
			CFStringRef filePathRef = CFURLCopyPath(appUrlRef);
			const char *filePath = CFStringGetCStringPtr(filePathRef, kCFStringEncodingUTF8);
			fs::path p(filePath);
			p = p.parent_path();
			p = p.parent_path();
			p = p.parent_path();
			systemParams.resourceDir = p;
			systemParams.resourceDir.make_preferred();
			systemParams.resourceDir.append("resources");
			// TODO 2021.08.17
			// get directory that app can read/write
			// systemParams.workingDir = systemParams.resourceDir;
#endif
		}

		////
		// load config.json
		{
			fs::path configPath(systemParams.workingDir);
			configPath.append("config.json");
			if (!fs::exists(configPath))
			{
				fs::path defaultConfigPath(systemParams.resourceDir);
				defaultConfigPath.append("defaultConfig.json");
				fs::copy_file(defaultConfigPath, configPath);
				std::cout << "No config file found. We use default config file." << std::endl;
			}
			// config
			// parse config json
			std::ifstream ifs(configPath);
			configFileContent = nlohmann::json::parse(ifs);
			ifs.close();
			config = configFileContent;
		}

		////
		// log
		if (config.contains("log"))
		{
			nlohmann::json &logJson = config.at("log");
			if (!logJson.contains("dir") || logJson.at("dir").get<std::string>().size() == 0)
			{
				fs::path logsDir = systemParams.workingDir;
				logsDir.append("log");
				fs::create_directories(logsDir);
				logJson["dir"] = logsDir.string();
			}
			// initialize logger for Core
			logger.initialize("Core", config.at("log"));
		}

		////
		// output
		if (config.contains("output"))
		{
			nlohmann::json &outputJson = config.at("output");
			// output["type"] == "local"|"download"
			if (outputJson.contains("type") && outputJson.at("type").get<std::string>() == "local")
			{
				nlohmann::json &outputLocalJson = outputJson.at("local");
				if (!outputLocalJson.contains("dir") || outputLocalJson.at("dir").get<std::string>().size() == 0)
				{
					fs::path outputsDir = systemParams.workingDir;
					outputsDir.append("output");
					fs::create_directories(outputsDir);
					outputLocalJson["dir"] = outputsDir.string();
				}
			}
		}

		////
		// plugin
		if (config.contains("plugin"))
		{
			nlohmann::json &pluginJson = config.at("plugin");
			// directory
			if (!pluginJson.contains("dir") || pluginJson.at("dir").get<std::string>().size() == 0)
			{
				fs::path pluginDir = systemParams.workingDir;
				pluginDir.append("plugin");
				fs::create_directories(pluginDir);
				pluginJson["dir"] = pluginDir.string();
			}
			// download and parse plugin list
			if (pluginJson.contains("listURL"))
			{
				nlohmann::json pluginListJson = nlohmann::json::object();
				for (const auto &listUrl : pluginJson.at("listURL"))
				{
					fs::path listJsonPath(pluginJson.at("dir").get<std::string>());
					listJsonPath.make_preferred();
					listJsonPath.append("tmp.json");
					Util::download(listUrl.get<std::string>(), listJsonPath);
					std::ifstream ifs(listJsonPath);
					nlohmann::json listJson = nlohmann::json::parse(ifs);
					ifs.close();
					fs::remove_all(listJsonPath);
					for (const auto &el : listJson.items())
					{
						pluginListJson[el.key()] = el.value();
					}
				}
				pluginJson["list"] = pluginListJson;
			}

			// initialize Doppelganger::Plugin instances
			for (const auto &pluginEntry : pluginJson["list"].items())
			{
				const std::string &name = pluginEntry.key();
				const nlohmann::json &pluginInfo = pluginEntry.value();
				const std::shared_ptr<Doppelganger::Plugin> plugin_ = std::make_shared<Doppelganger::Plugin>(shared_from_this(), name, pluginInfo);
				plugin[name] = plugin_;
			}

			// install plugins
			// installed plugins are stored in config["plugin"]["dir"]/installed.json
			fs::path installedPluginJsonPath(config.at("plugin").at("dir").get<std::string>());
			installedPluginJsonPath.append("installed.json");
			if (fs::exists(installedPluginJsonPath))
			{
				std::ifstream ifs(installedPluginJsonPath);
				const nlohmann::json installedPluginJson = nlohmann::json::parse(ifs);
				ifs.close();

				for (const auto &pluginToBeInstalled : installedPluginJson.items())
				{
					const std::string &name = pluginToBeInstalled.key();
					const nlohmann::json &pluginInfo = pluginToBeInstalled.value();
					const std::string &version = pluginInfo.at("version").get<std::string>();

					if (plugin.find(name) != plugin.end())
					{
						plugin.at(name)->install(version);
					}
					else
					{
						std::stringstream ss;
						ss << "Plugin \"";
						ss << name;
						ss << "\" (";
						ss << version;
						ss << ")";
						ss << " is NOT found.";
						logger.log(ss.str(), "ERROR");
					}
				}
			}
			else
			{
				const nlohmann::json emptyObject = nlohmann::json::object();
				std::ofstream ofs(installedPluginJsonPath);
				ofs << emptyObject.dump(4);
				ofs.close();
			}

			// install non-optional plugins
			for (const auto &pluginEntry : pluginJson.at("list").items())
			{
				const std::string &name = pluginEntry.key();
				const nlohmann::json &pluginInfo = pluginEntry.value();
				const bool &optional = pluginInfo.at("optional").get<bool>();
				if (!optional)
				{
					if (plugin.find(name) != plugin.end())
					{
						if (plugin.at(name)->installedVersion.size() == 0)
						{
							plugin.at(name)->install(std::string("latest"));
						}
					}
					else
					{
						std::stringstream ss;
						ss << "Non-optional plugin \"";
						ss << name;
						ss << "\" (latest)";
						ss << " is NOT found.";
						logger.log(ss.str(), "ERROR");
					}
				}
			}
		}

		// initialize acceptor
		if (config.contains("server"))
		{
			nlohmann::json &serverJson = config.at("server");

			boost::system::error_code ec;
			boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(serverJson.at("host").get<std::string>()), serverJson.at("port").get<int>());

			// Open the acceptor
			acceptor.open(endpoint.protocol(), ec);
			if (ec)
			{
				fail(ec, "open");
				return;
			}

			// Allow address reuse
			acceptor.set_option(boost::asio::socket_base::reuse_address(true));
			if (ec)
			{
				fail(ec, "set_option");
				return;
			}

			// Bind to the server address
			acceptor.bind(endpoint, ec);
			if (ec)
			{
				fail(ec, "bind");
				return;
			}

			// Start listening for connections
			acceptor.listen(
				boost::asio::socket_base::max_listen_connections, ec);
			if (ec)
			{
				fail(ec, "listen");
				return;
			}

			serverJson["port"] = acceptor.local_endpoint().port();
			std::stringstream completeURL;
			completeURL << serverJson.at("protocol").get<std::string>();
			completeURL << serverJson.at("host").get<std::string>();
			completeURL << ":";
			completeURL << serverJson.at("port").get<int>();
			serverJson["completeURL"] = completeURL.str();
		}
	}

	void Core::run()
	{
		// start servers
		{
			acceptor.async_accept(
				socket,
				[self = shared_from_this()](boost::system::error_code ec)
				{
					self->onAccept(ec);
				});

			std::stringstream s;
			s << "Listening for requests at : " << config.at("server").at("completeURL").get<std::string>();
			logger.log(s.str(), "SYSTEM");
		}
		////
		// open browser
		// "openOnBoot" = true|false
		// "browser" = "chrome"|"firefox"|"edge"|"safari"|"default"|"N/A"
		// "browserPath" = path/to/executables
		// "openAs" = "app"|"window"|"tab"|"default"
		// "cmd" = "command to be executed. use response["cmd"]+URL"
		if (config.contains("browser"))
		{
			nlohmann::json &browserJson = config.at("browser");
			// by default, we open browser
			if (!browserJson.contains("openOnBoot") || browserJson.at("openOnBoot").get<bool>())
			{
				browserJson["openOnBoot"] = true;
				// by default, we use default browser (open/start command)
				std::string browserType = std::string("default");
				if (browserJson.contains("type"))
				{
					const std::string type = browserJson.at("type").get<std::string>();
					if (type == "chrome" || type == "firefox" || type == "edge" || type == "safari")
					{
						browserType = type;
					}
				}
				browserJson["type"] = browserType;

				// path
				if (browserJson.contains("path") && browserJson.at("path").get<std::string>().size() > 0)
				{
					// if browser is specified by the path, we ignore "type"
					browserJson.at("type") = std::string("N/A");
					// possibly, we need to verify the path ...
				}
				// update browserJson["path"]
				if (browserJson.at("type").get<std::string>() == "chrome")
				{
#if defined(_WIN32) || defined(_WIN64)
					std::vector<fs::path> chromePaths({fs::path("C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe"),
													   fs::path("C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe")});
#elif defined(__APPLE__)
					std::vector<fs::path> chromePaths({fs::path("/Applications/Google Chrome.app/Contents/MacOS/Google Chrome")});
#endif
					for (auto &p : chromePaths)
					{
						p.make_preferred();
						if (fs::exists(p))
						{
							browserJson["path"] = p.string();
							break;
						}
					}
				}
				else if (browserJson.at("type").get<std::string>() == "firefox")
				{
#if defined(_WIN32) || defined(_WIN64)
					std::vector<fs::path> firefoxPaths({fs::path("C:\\Program Files\\Mozilla Firefox\\firefox.exe"),
														fs::path("C:\\Program Files (x86)\\Mozilla Firefox\\firefox.exe")});
#elif defined(__APPLE__)
					// todo update
					std::vector<fs::path> firefoxPaths({fs::path("/Applications/Google Chrome.app/Contents/MacOS/Google Chrome")});
#endif
					for (auto &p : firefoxPaths)
					{
						p.make_preferred();
						if (fs::exists(p))
						{
							browserJson["path"] = p.string();
							break;
						}
					}
				}
				else if (browserJson.at("type").get<std::string>() == "edge")
				{
					// only for windows
					fs::path edgePath("C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe");
					edgePath.make_preferred();
					browserJson["path"] = edgePath.string();
				}
				else if (browserJson.at("type").get<std::string>() == "safari")
				{
					// todo
					browserJson["path"] = std::string("");
				}
				else if (browserJson.at("type").get<std::string>() == "default")
				{
					// do nothing
#if defined(_WIN32) || defined(_WIN64)
					browserJson["path"] = std::string("");
#elif defined(__APPLE__)
					// todo update
					browserJson["path"] = std::string("open");
#endif
				}
				else
				{
					// N/A
					//   do nothing
				}

				////
				// update openAs
				if (!browserJson.contains("openAs") || browserJson.at("openAs").get<std::string>().size() == 0)
				{
					browserJson["openAs"] = std::string("default");
				}
				// update browserJson["openAs"]
				if (browserJson.at("type").get<std::string>() == "chrome")
				{
					// chrome supports "app"|"window"|"tab"|"default"
					//   precisely speaking, "tab" and "default" are the same
				}
				else if (browserJson.at("type").get<std::string>() == "firefox")
				{
					// firefox supports "window"|"tab"|"default"
					//   precisely speaking, "tab" and "default" are the same
					if (browserJson.at("openAs").get<std::string>() == "app")
					{
						browserJson.at("openAs") = std::string("window");
					}
				}
				else if (browserJson.at("type").get<std::string>() == "edge")
				{
					// edge supports "app"|"window"|"tab"|"default"
					//   precisely speaking, "tab" and "default" are the same
				}
				else if (browserJson.at("type").get<std::string>() == "safari")
				{
					// todo check which mode is supported by safari ...
					// safari supports "app"|"window"|"tab"|"default"
					//   precisely speaking, "tab" and "default" are the same
				}
				else if (browserJson.at("type").get<std::string>() == "default")
				{
					// for default browser (start/open command), we cannot know which type is supported ...
					browserJson.at("openAs") = std::string("default");
				}
				else
				{
					// for N/A (specified by path), we cannot know which which type is supported ...
					browserJson.at("openAs") = std::string("default");
				}

				////
				// prepare cmd
				// todo: check command line switches for each browser
				std::stringstream cmd;
				// browser path
#if defined(_WIN32) || defined(_WIN64)
				// make sure that program runs in background.
				if (browserJson.at("type").get<std::string>() != "default")
				{
					cmd << "start \"\"";
					cmd << " ";
				}
#endif
				cmd << "\"";
				cmd << browserJson.at("path").get<std::string>();
				cmd << "\"";
				cmd << " ";
				if (browserJson.at("openAs").get<std::string>() == "app")
				{
					cmd << "--app=";
				}
				else if (browserJson.at("openAs").get<std::string>() == "window")
				{
					cmd << "--new-window";
					cmd << " ";
				}
				else if (browserJson.at("openAs").get<std::string>() == "tab")
				{
					// no command line switch
				}
				else // default
				{
					// no command line switch
				}
				// URL
				cmd << config.at("server").at("completeURL").get<std::string>();
				browserJson["cmd"] = cmd.str();
				logger.log(cmd.str(), "SYSTEM");
				system(cmd.str().c_str());
			}
		}
	}

	void Core::fail(
		boost::system::error_code ec,
		char const *what)
	{
		// Don't report on canceled operations
		if (ec == boost::asio::error::operation_aborted)
		{
			return;
		}

		std::stringstream s;
		s << what << ": " << ec.message();
		logger.log(s.str(), "ERROR");
	}

	void Core::onAccept(
		boost::system::error_code ec)
	{
		if (ec)
		{
			return fail(ec, "accept");
		}
		else
		{
			// Launch a new session for this connection
			std::make_shared<HTTPSession>(std::move(socket), shared_from_this())->run();
		}

		// Accept another connection
		acceptor.async_accept(
			socket,
			[self = shared_from_this()](boost::system::error_code ec)
			{
				self->onAccept(ec);
			});
	}

}

#endif
