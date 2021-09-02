#ifndef CORE_CPP
#define CORE_CPP

#include "Core.h"
#include "Logger.h"
#include "Room.h"
#include "HTTPSession.h"
#include "Plugin.h"
#include "Util.h"

#include <fstream>
#include <sstream>
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
		std::ifstream ifs(pathConfig);
		config = nlohmann::json::parse(ifs);
		ifs.close();

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
		if (config.contains("pluginListURL"))
		{
			nlohmann::json pluginListJson = nlohmann::json::object();
			for (const auto &listUrl : config.at("pluginListURL"))
			{
				fs::path listJsonPath(config.at("pluginDir").get<std::string>());
				listJsonPath.make_preferred();
				listJsonPath.append("tmp.json");
				Util::download(shared_from_this(), listUrl.get<std::string>(), listJsonPath);
				std::ifstream ifs(listJsonPath);
				nlohmann::json listJson = nlohmann::json::parse(ifs);
				ifs.close();
				fs::remove_all(listJsonPath);
				// the same code as range for
				for (const auto &el : listJson.items())
				{
					pluginListJson[el.key()] = el.value();
				}
			}
			config["pluginList"] = pluginListJson;
		}
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

		// initialize logger
		{
			logger.initialize("Core", config);
		}

		// install plugin
		if (config.contains("installedPlugin") && config.at("installedPlugin").size() != 0)
		{
			for (const auto &pluginToBeInstalled : config.at("installedPlugin"))
			{
				const std::string name = pluginToBeInstalled.at("name").get<std::string>();
				const std::string version = pluginToBeInstalled.at("version").get<std::string>();
				if (config.at("pluginList").contains(name))
				{
					if (config.at("pluginList").at(name).at("versions").contains(version))
					{
						const std::shared_ptr<Doppelganger::Plugin> plugin_ = std::make_shared<Doppelganger::Plugin>(shared_from_this());
						plugin_->loadPlugin(name, config.at("pluginList").at(name).at("versions").at(version).get<std::string>());
						// https://n-taka.info/nextcloud/s/R985TytEobs6D8A/download/browserInfo.zip
						plugin[name] = plugin_;
					}
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
		}
		// open browser (if needed)
		// todo
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
