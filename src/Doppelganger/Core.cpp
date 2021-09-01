#ifndef CORE_CPP
#define CORE_CPP

#include "Core.h"
#include "Logger.h"
#include "Room.h"
#include "HTTPSession.h"
#include "Plugin.h"

#include <fstream>
#include <nlohmann/json.hpp>

namespace Doppelganger
{
	Core::Core(boost::asio::io_context &ioc_)
		: ioc(ioc_), acceptor(ioc), socket(ioc)
	{
	}

	void Core::parseConfig(
		const fs::path &pathConfig,
		const fs::path &workingDir,
		nlohmann::json &config)
	{
		// parse json
		std::ifstream ifs(pathConfig.string());
		config = nlohmann::json::parse(ifs);

		// set default values for missing configuration
		////
		// protocol
		if (!config.contains("protocol") || config.at("protocol").get<std::string>().size() == 0)
		{
			config["protocol"] = "http://";
		}
		////
		// host
		if (!config.contains("host") || config.at("host").get<std::string>().size() == 0)
		{
			config["host"] = "127.0.0.1";
		}
		////
		// port
		if (!config.contains("port") || config.at("port").get<int>() < 1024)
		{
			config["port"] = 0;
		}
		////
		// log (we use one directory per room)
		if (!config.contains("logsDir") || config.at("logsDir").get<std::string>().size() == 0)
		{
			fs::path logsDir = workingDir;
			logsDir.append("log");
			fs::create_directories(logsDir);
			config["logsDir"] = logsDir.string();
		}
		////
		// output (we use one directory per room)
		if (!config.contains("outputsDir") || config.at("outputsDir").get<std::string>().size() == 0)
		{
			fs::path outputsDir = workingDir;
			outputsDir.append("output");
			fs::create_directories(outputsDir);
			config["outputsDir"] = outputsDir.string();
		}
		////
		// plugin (we use one directory (multiple rooms share the directory))
		//     it is more useful the previous setting is kept even if user reboot Doppelganger.
		//     last settings are stored in (cached) config.json
		if (!config.contains("pluginDir") || config.at("pluginDir").get<std::string>().size() == 0)
		{
			fs::path pluginDir = workingDir;
			pluginDir.append("plugin");
			fs::create_directories(pluginDir);
			config["pluginDir"] = pluginDir.string();
		}
		////
		// install plugin
		if (config.contains("installedPlugin") && config.at("installedPlugin").size() != 0)
		{
			for (const auto &plugin : config.at("installedPlugin"))
			{
				const std::string name = plugin.at("name").get<std::string>();
				const std::string version = plugin.at("version").get<std::string>();
				std::cout << name << " (" << version << ")" << std::endl;
			}
			// todo 2021.08.24
		}

		////
		// browserInfo
		// todo

		////
		// FreeCAD
		if (!config.contains("FreeCADPath") || config.at("FreeCADPath").get<std::string>().size() == 0)
		{
#if defined(_WIN32) || defined(_WIN64)
			std::vector<fs::path> ProgramFilesPaths(
				{fs::path("C:\\Program Files"),
				 fs::path("C:\\Program Files (x86)")});
			const std::string FreeCAD("FreeCAD");
			fs::path FreeCADPath("");
			for (const auto &ProgramFiles : ProgramFilesPaths)
			{
				if (!FreeCADPath.empty())
				{
					break;
				}
				for (const auto &p : fs::directory_iterator(ProgramFiles))
				{
					if (!FreeCADPath.empty())
					{
						break;
					}
					const std::string &programName = p.path().filename().string();
					if (FreeCAD == programName)
					{
						FreeCADPath = p;
						FreeCADPath.append("bin");
						FreeCADPath.append("FreeCADCmd.exe");
					}
				}
			}
#elif defined(__APPLE__)
			// todo
			fs::path FreeCADPath("/Applications/FreeCAD.app/Contents/Resources/bin/FreeCADCmd");
#endif
			FreeCADPath.make_preferred();
			config["FreeCADPath"] = FreeCADPath.string();
		}
	}

	void Core::setup()
	{
		//////
		// initialize system parameters

		// setup for resources
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
			parseConfig(configPath, systemParams.workingDir, config);
		}

		// load pluginList.json
		{
			fs::path configPath(systemParams.workingDir);
			configPath.append("pluginList.json");
			if (!fs::exists(configPath))
			{
				fs::path defaultConfigPath(systemParams.resourceDir);
				defaultConfigPath.append("defaultPluginList.json");
				fs::copy_file(defaultConfigPath, configPath);
				std::cout << "No pluginList file found. We use default pluginList file." << std::endl;
			}
		}

		// initialize logger
		{
			logger.initialize("Core", config);
		}

		// initialize acceptor
		{
			boost::system::error_code ec;

			boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(config.at("host").get<std::string>()), config.at("port").get<int>());

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

			config["port"] = acceptor.local_endpoint().port();
			std::stringstream completeURL;
			completeURL << config.at("protocol").get<std::string>();
			completeURL << config.at("host").get<std::string>();
			completeURL << ":";
			completeURL << config.at("port").get<int>();
			config["completeURL"] = completeURL.str();
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
			s << "Listening for requests at : " << config.at("completeURL").get<std::string>();
			logger.log(s.str(), "SYSTEM");

			// debug
			std::shared_ptr<Doppelganger::Plugin> plugin = std::make_shared<Doppelganger::Plugin>(shared_from_this());
			plugin->loadPlugin("browserInfo", "https://n-taka.info/nextcloud/s/R985TytEobs6D8A/download/browserInfo.zip");
			// https://n-taka.info/nextcloud/s/R985TytEobs6D8A/download/browserInfo.zip
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
