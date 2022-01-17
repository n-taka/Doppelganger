#ifndef CORE_CPP
#define CORE_CPP

#include "Doppelganger/Core.h"
#include "Doppelganger/Logger.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/HTTPSession.h"
#include "Doppelganger/Plugin.h"
#include "Doppelganger/Listener.h"
#include "Doppelganger/Util/download.h"
#include "Doppelganger/Util/getCurrentTimestampAsString.h"

#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <cstddef>
#include <memory>

#include <fstream>
#include <sstream>
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

		////
		// config.json
		nlohmann::json config;
		{
			std::lock_guard<std::mutex> lock(mutexConfig);
			getCurrentConfig(config);
		}

		////
		// directory for Core
		// Doppelganger/data/YYYYMMDDTHHMMSS-Core/
		{
			dataDir = DoppelgangerRootDir;
			dataDir.append("data");
			std::string dirName("");
			dirName += Util::getCurrentTimestampAsString(false);
			dirName += "-Core";
			dataDir.append(dirName);
			fs::create_directories(dataDir);
		}

		////
		// log
		// Doppelganger/data/YYYYMMDDTHHMMSS-Core/log
		if (config.contains("log"))
		{
			logger.initialize(shared_from_this());
		}

		////
		// output
		// Doppelganger/data/YYYYMMDDTHHMMSS-Core/output
		if (config.contains("output"))
		{
			fs::path outputDir = dataDir;
			outputDir.append("output");
			fs::create_directories(outputDir);
		}

		////
		// plugin
		if (config.contains("plugin"))
		{
			const nlohmann::json &pluginJson = config.at("plugin");

			fs::path pluginsDir = DoppelgangerRootDir;
			pluginsDir.append("plugin");
			fs::create_directories(pluginsDir);
		}

		// initialize server (and certificates)
		if (config.contains("server"))
		{
			const nlohmann::json &serverJson = config.at("server");

			boost::system::error_code ec;

			boost::asio::ip::tcp::resolver resolver(ioc_);
			boost::asio::ip::tcp::resolver::results_type endpointIterator = resolver.resolve(serverJson.at("host").get<std::string>(), std::to_string(serverJson.at("port").get<int>()), ec);
			if (ec)
			{
				{
					std::stringstream ss;
					ss << "Fail to resolve hostname \"" << serverJson.at("host").get<std::string>() << "\"";
					logger.log(ss.str(), "ERROR");
				}
				{
					std::stringstream ss;
					ss << ec.message();
					logger.log(ss.str(), "ERROR");
				}
				return;
			}

			boost::asio::ip::tcp::endpoint endpoint = endpointIterator->endpoint();

			protocol = serverJson.at("protocol").get<std::string>();
			if (protocol == "https")
			{
				if (serverJson.contains("certificateFiles"))
				{
					fs::path certificatePath, privateKeyPath;
					if (serverJson.at("certificateFiles").contains("certificate"))
					{
						certificatePath = fs::path(serverJson.at("certificateFiles").at("certificate").get<std::string>());
					}
					if (serverJson.at("certificateFiles").contains("privateKey"))
					{
						privateKeyPath = fs::path(serverJson.at("certificateFiles").at("privateKey").get<std::string>());
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
							logger.log(ss.str(), "ERROR");
						}
						{
							std::stringstream ss;
							ss << "    certificate: ";
							ss << certificatePath;
							logger.log(ss.str(), "ERROR");
						}
						{
							std::stringstream ss;
							ss << "    privateKey: ";
							ss << privateKeyPath;
							logger.log(ss.str(), "ERROR");
						}
						protocol = std::string("http");
					}
				}
				else
				{
					protocol = std::string("http");
				}
			}
			listener = std::make_shared<Listener>(shared_from_this(), ioc_, ctx_, endpoint);
		}
	}

	void Core::run()
	{
		nlohmann::json config;
		{
			std::lock_guard<std::mutex> lock(mutexConfig);
			getCurrentConfig(config);
		}

		// start servers
		{
			completeURL = protocol;
			completeURL += "://";
			completeURL += config.at("server").at("host").get<std::string>();
			completeURL += ":";
			completeURL += std::to_string(listener->acceptor_.local_endpoint().port());
		}
		{
			listener->run();

			std::stringstream s;
			s << "Listening for requests at : " << completeURL;
			logger.log(s.str(), "SYSTEM");
		}
		////
		// open browser
		// "openOnStartup" = true|false
		// "browser" = "chrome"|"firefox"|"edge"|"safari"|"default"|"N/A"
		// "browserPath" = path/to/executables
		// "openAs" = "app"|"window"|"tab"|"default"
		// "cmd" = "command to be executed. use response["cmd"]+URL"
		if (config.contains("browser"))
		{
			nlohmann::json &browserJson = config.at("browser");
			// by default, we open browser
			if ((!browserJson.contains("openOnStartup") || browserJson.at("openOnStartup").get<bool>()) && config.at("server").at("host") == "127.0.0.1")
			{
				browserJson["openOnStartup"] = true;
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
							browserJson["path"] = p.string();
							break;
						}
					}
				}
				else if (browserJson.at("type").get<std::string>() == "firefox")
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
					browserJson["path"] = std::string("/Applications/Safari.app");
				}
				else if (browserJson.at("type").get<std::string>() == "default")
				{
					// do nothing
					browserJson["path"] = std::string("");
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
					if (browserJson.at("openAs").get<std::string>() == "app" || browserJson.at("openAs").get<std::string>() == "window")
					{
						browserJson.at("openAs") = std::string("tab");
					}
					// safari supports "tab"|"default"
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
				std::stringstream cmd;
				// browser path
#if defined(_WIN64)
				// make sure that program runs in background.
				cmd << "start ";
				cmd << "\"\" ";
				if (browserJson.at("type").get<std::string>() != "default")
				{
					cmd << "\"";
					cmd << browserJson.at("path").get<std::string>();
					cmd << "\"";
					cmd << " ";
				}
#elif defined(__APPLE__)
				if (browserJson.at("type").get<std::string>() == "default")
				{
					cmd << "open ";
				}
				else if (browserJson.at("type").get<std::string>() == "safari")
				{
					cmd << "open -a Safari.app ";
				}
				else
				{
					cmd << "\"";
					cmd << browserJson.at("path").get<std::string>();
					cmd << "\"";
					cmd << " ";
				}
#elif defined(__linux__)
				if (browserJson.at("type").get<std::string>() == "default")
				{
					cmd << "xdg-open ";
				}
				else
				{
					cmd << "\"";
					cmd << browserJson.at("path").get<std::string>();
					cmd << "\"";
					cmd << " ";
				}
#endif
				if (browserJson.at("openAs").get<std::string>() == "app")
				{
					cmd << "--app=";
				}
				else if (browserJson.at("openAs").get<std::string>() == "window")
				{
					cmd << "--new-window ";
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
				cmd << completeURL;
#if defined(__APPLE__)
				if (browserJson.at("type").get<std::string>() != "default" && browserJson.at("type").get<std::string>() != "safari")
				{
					cmd << " &";
				}
#elif defined(__linux__)
				if (browserJson.at("type").get<std::string>() != "default")
				{
					cmd << " &";
				}
#endif

				browserJson["cmd"] = cmd.str();
				logger.log(cmd.str(), "SYSTEM");
				system(cmd.str().c_str());
			}
		}
	}

	void Core::getCurrentConfig(nlohmann::json &config) const
	{
		fs::path configPath(DoppelgangerRootDir);
		configPath.append("config.json");
		if (fs::exists(configPath))
		{
			// we could directly use std::filesystem::path, but we could not directly boost::filesystem::path
			std::ifstream ifs(configPath.string());
			config = nlohmann::json::parse(ifs);
			ifs.close();
		}
		else
		{
			// no config file is found. (e.g. use Doppelganger first time)
			generateDefaultConfigJson(config);
			std::cout << "No config file found. We use default config." << std::endl;
			updateConfig(config);
		}
	}

	void Core::updateConfig(const nlohmann::json &config) const
	{
		fs::path configPath(DoppelgangerRootDir);
		configPath.append("config.json");
		std::ofstream ofs(configPath.string());
		ofs << config.dump(4);
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

	void Core::getPluginCatalogue(
		const nlohmann::json &listURLJson,
		nlohmann::json &pluginCatalogue)
	{
		pluginCatalogue = nlohmann::json::object();

		for (const auto &listUrl : listURLJson)
		{
			fs::path listJsonPath(DoppelgangerRootDir);
			listJsonPath.append("plugin");
			listJsonPath.append("tmp.json");
			Util::download(listUrl.get<std::string>(), listJsonPath);
			std::ifstream ifs(listJsonPath.string());
			if (ifs)
			{
				nlohmann::json listJson = nlohmann::json::parse(ifs);
				ifs.close();
				fs::remove_all(listJsonPath);
				for (const auto &el : listJson.items())
				{
					pluginCatalogue[el.key()] = el.value();
				}
			}
		}
	}

	void Core::generateDefaultConfigJson(nlohmann::json &config)
	{
		config = nlohmann::json::object();
		{
			config["browser"] = nlohmann::json::object();
			{
				config["browser"]["openAs"] = std::string("default");
				config["browser"]["openOnStartup"] = true;
				config["browser"]["path"] = std::string("");
				config["browser"]["type"] = std::string("default");
			}
			config["log"] = nlohmann::json::object();
			{
				config["log"]["level"] = nlohmann::json::array();
				{
					config["log"]["level"].push_back(std::string("SYSTEM"));
					config["log"]["level"].push_back(std::string("APICALL"));
					config["log"]["level"].push_back(std::string("WSCALL"));
					config["log"]["level"].push_back(std::string("ERROR"));
					config["log"]["level"].push_back(std::string("MISC"));
					config["log"]["level"].push_back(std::string("DEBUG"));
				}
				config["log"]["type"] = nlohmann::json::array();
				{
					config["log"]["type"].push_back(std::string("STDOUT"));
					config["log"]["type"].push_back(std::string("FILE"));
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
				config["plugin"]["installed"] = nlohmann::json::object();
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
			}
		}
	}
}

#endif
