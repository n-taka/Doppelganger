#ifndef CORE_CPP
#define CORE_CPP

#include "Core.h"
#include "Logger.h"
#include "Room.h"
#include "HTTPSession.h"
// #include "PluginLoader.h"

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
		// logDir
		if (!config.contains("logDir") || config.at("logDir").get<std::string>().size() == 0)
		{
			if (!config.contains("logsDir") || config.at("logsDir").get<std::string>().size() == 0)
			{
				fs::path logsDir = workingDir;
				logsDir.append("log");
				config["logsDir"] = logsDir.string();
			}
			fs::path logDir(config.at("logsDir").get<std::string>());
			logDir.append(Logger::getCurrentTimestampAsString(false));
			config["logDir"] = logDir.string();
		}
		////
		// outputDir
		if (!config.contains("outputDir") || config.at("outputDir").get<std::string>().size() == 0)
		{
			if (!config.contains("outputsDir") || config.at("outputsDir").get<std::string>().size() == 0)
			{
				fs::path outputsDir = workingDir;
				outputsDir.append("output");
				config["outputsDir"] = outputsDir.string();
			}
			fs::path outputDir(config.at("outputsDir").get<std::string>());
			outputDir.append(Logger::getCurrentTimestampAsString(false));
			config["outputDir"] = outputDir.string();
		}
		////
		// browser
		if (!config.contains("browser"))
		{
			config["browser"] = nlohmann::json::object();
		}
		if (!config.at("browser").contains("path") || config.at("browser").at("path").get<std::string>().size() == 0)
		{
			if (!config.at("browser").contains("openAs") || config.at("browser").at("openAs").get<std::string>().size() == 0)
			{
				config.at("browser")["openAs"] = "app";
			}

			if (!config.at("browser").contains("type") || config.at("browser").at("type").get<std::string>().size() == 0)
			{
				config.at("browser")["type"] = "chrome";
			}
			if (config.at("browser").at("type").get<std::string>() == "chrome")
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
				;
			}
			else if (config.at("browser").at("type").get<std::string>() == "safari")
			{
				// todo
				config.at("browser")["path"] = "";

				if (config.at("browser").at("openAs").get<std::string>() == "app")
				{
					// firefox doesn't support app mode
					config.at("browser")["openAs"] = "window";
				}
			}
			else if (config.at("browser").at("type").get<std::string>() == "firefox")
			{
#if defined(_WIN32) || defined(_WIN64)
				std::vector<fs::path> firefoxPaths({fs::path("C:\\Program Files\\Mozilla Firefox\\firefox.exe"),
													fs::path("C:\\Program Files (x86)\\Mozilla Firefox\\firefox.exe")});
#elif defined(__APPLE__)
				std::vector<fs::path> firefoxPaths({fs::path("/Applications/Google Chrome.app/Contents/MacOS/Google Chrome")});
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

				if (config.at("browser").at("openAs").get<std::string>() == "app")
				{
					// firefox doesn't support app mode
					config.at("browser")["openAs"] = "window";
				}
			}
		}
		if (!config.at("browser").contains("openOnBoot"))
		{
			config.at("browser")["openOnBoot"] = true;
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
		////
		// plugin repo.
		// * no default value
#if 0
		if (!config.contains("pluginRepositories") || config.at("pluginRepositories").size() == 0)
		{
		}
#endif
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
				std::cout << "no config file found. we use default config file" << std::endl;
			}
			// config
			parseConfig(configPath, systemParams.workingDir, config);
		}

		// initialize logger
		{
			fs::path logDir(config.at("logDir").get<std::string>());
			std::unordered_map<std::string, bool> logLevel;
			for (const auto &level : config.at("logLevel"))
			{
				logLevel[level.get<std::string>()] = true;
			}
			std::unordered_map<std::string, bool> logType;
			for (const auto &type : config.at("logType"))
			{
				logType[type.get<std::string>()] = true;
			}
			Logger::getInstance().initialize(logDir, logLevel, logType);
		}

		// initialize plugin
		// {
		// 	fs::path dirName(pantry->systemParams.baseDir);
		// 	dirName /= "resources";
		// 	dirName /= "plugin";
		// 	PluginLoader loader(dirName);
		// 	std::vector<std::string> plugins;
		// 	loader.loadPlugins(pantry);
		// }

		// 	//////
		// 	// initialize internal API TODO 2021.08.17
		// 	//////
		// 	{
		// 		std::function<void(const fs::path &, Doppel::triangleMesh &)> loadTexture = Doppel::util::loadTexture;
		// 		internalAPI["loadTexture"] = loadTexture;

		// 		std::function<void(const Doppel::triangleMesh &, Doppel::triangleMesh &)> projectMeshAttributes = Doppel::util::projectMeshAttributes;
		// 		internalAPI["projectMeshAttributes"] = projectMeshAttributes;

		// 		std::function<void(const fs::path &, const std::string &, const bool &, const bool &, const double &, const fs::path &, const fs::path &, std::vector<Doppel::triangleMesh> &)> readMeshFromFile = Doppel::util::readMeshFromFile;
		// 		internalAPI["readMeshFromFile"] = readMeshFromFile;

		// 		std::function<void(const fs::path &, const std::string &, const bool &, std::vector<Doppel::triangleMesh> &)> readMeshFromPolygonFile = Doppel::util::readMeshFromPolygonFile;
		// 		internalAPI["readMeshFromPolygonFile"] = readMeshFromPolygonFile;

		// 		std::function<void(const fs::path &, const Doppel::triangleMesh &, bool)> writeMeshToFile = Doppel::util::writeMeshToFile;
		// 		internalAPI["writeMeshToFile"] = writeMeshToFile;

		// 		std::function<void(const int &, const Doppel::triangleMesh &, const fs::path &)> saveTexture = Doppel::util::saveTexture;
		// 		internalAPI["saveTexture"] = saveTexture;

		// 		std::function<void(const int &, const Doppel::triangleMesh &, std::vector<unsigned char> &)> saveTextureToMemory = Doppel::util::saveTextureToMemory;
		// 		internalAPI["saveTextureToMemory"] = saveTextureToMemory;

		// 		std::function<void(const std::vector<int> &, const nlohmann::json &, const nlohmann::json &, std::unordered_map<int, Doppel::triangleMesh> &)> updateMeshesFromJson = Doppel::util::updateMeshesFromJson;
		// 		internalAPI["updateMeshesFromJson"] = updateMeshesFromJson;

		// 		std::function<fs::path(const nlohmann::json &)> writeBase64ToFile = Doppel::util::writeBase64ToFile;
		// 		internalAPI["writeBase64ToFile"] = writeBase64ToFile;

		// 		std::function<void(const int &, const std::unordered_map<int, Doppel::triangleMesh> &, nlohmann::json &, const bool)> writeMeshToJson = Doppel::util::writeMeshToJson;
		// 		internalAPI["writeMeshToJson"] = writeMeshToJson;

		// 		std::function<void(const std::unordered_map<std::string, boost::any> &, const triangleMesh &, const std::string &, std::vector<unsigned char> &, std::vector<unsigned char> &, std::vector<unsigned char> &)> writeMeshToMemory = Doppel::util::writeMeshToMemory;
		// 		internalAPI["writeMeshToMemory"] = writeMeshToMemory;
		// 	}

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
			Logger::getInstance().log(s.str(), "SYSTEM");
		}

#if 0
		// open browser
		{
			if (config.at("browser").contains("openOnBoot") && config.at("browser").at("openOnBoot").get<bool>())
			{
				// todo here

				// the code below is redundant (see GET serverInfo)
				// installation check (Chrome)
				// 				std::stringstream cmd;
				// #if defined(_WIN32) || defined(_WIN64)
				// 				// we need to use "start "" blah blah..." to run in background
				// 				cmd << "start \"\" ";
				// 				if (pantry->systemParams.ChromeInstalled)
				// 				{
				// 					cmd << pantry->systemParams.ChromePath;
				// 					// cmd << " ";
				// 					// cmd << "--incognito";
				// 					cmd << " ";
				// 					cmd << "--app=";
				// 				}
				// 				cmd << pantry->serverParams.completeURL;
				// #elif defined(__APPLE__)
				// 				if (pantry->systemParams.ChromeInstalled)
				// 				{
				// 					cmd << pantry->systemParams.ChromePath;
				// 					// cmd << " ";
				// 					// cmd << "--incognito";
				// 					// cmd << " ";
				// 					// cmd << "--user-data-dir=";
				// 					// cmd << pantry->logParams.logDir.string();
				// 					// cmd << "/UserData";
				// 					cmd << " ";
				// 					cmd << "--app=";
				// 				}
				// 				else
				// 				{
				// 					cmd << "open ";
				// 				}
				// 				cmd << pantry->serverParams.completeURL;
				// 				cmd << " &";
				// #endif
				// 				std::cout << cmd.str() << std::endl;
				// 				system(cmd.str().c_str());
			}
		}
#endif
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
		Logger::getInstance().log(s.str(), "ERROR");
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
