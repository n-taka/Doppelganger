#ifndef SERVERINFO_CPP
#define SERVERINFO_CPP

#include "../plugin.h"

#include <string>
#if defined(_WIN32) || defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#endif

extern "C" DLLEXPORT void metadata(const std::shared_ptr<Doppelganger::Room> &room, const nlohmann::json &parameters, nlohmann::json &response)
{
	response["author"] = "Kazutaka Nakashima";
	response["version"] = 1.0;
}

extern "C" DLLEXPORT void pluginProcess(const std::shared_ptr<Doppelganger::Room> &room, const nlohmann::json &parameters, nlohmann::json &response)
{
	////
	// [IN]
	//    parameters: config json
	// [OUT]
	//    response["openOnBoot"] = true|false
	//    response["browser"] = "chrome"|"firefox"|"edge"|"safari"|"default"|"N/A"
	//    response["browserPath"] = path/to/executables
	//    response["openAs"] = "app"|"window"|"tab"|"none"
	//    response["cmd"] = "command to be executed. use response["cmd"]+URL"

	response = nlohmann::json::object();
	////
	// initialize by default values
	response["openOnBoot"] = true;
	response["browser"] = std::string("default");
	response["browserPath"] = std::string("");
	response["openAs"] = std::string("none");
	response["cmd"] = std::string("start ");

	////
	// by default, we open browser on boot.
	if (parameters.contains("openOnBoot") && !parameters.at("openOnBoot").get<bool>())
	{
		response.at("openOnBoot") = false;
		return;
	}

	////
	// by default, we use default browser (open/start command)
	if (parameters.contains("browser") && parameters.at("browser").get<std::string>().size() > 0)
	{
		const std::string browser = parameters.at("browser").get<std::string>();
		if (browser == "chrome" || browser == "firefox" || browser == "edge" || browser == "safari")
		{
			response.at("browser") = browser;
		}
	}

	////
	// browserPath
	if (parameters.contains("browserPath") && parameters.at("browserPath").get<std::string>().size() > 0)
	{
		// if browser is specified by browserPath, we ignore parameters["browser"]
		response.at("browser") = std::string("N/A");
		response.at("browserPath") = parameters.at("browserPath").get<std::string>();
		// possibly, we need to verify the path ...
	}
	// update response["browserPath"]
	if (response.at("browser").get<std::string>() == "chrome")
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
				response.at("browserPath") = p.string();
				break;
			}
		}
	}
	else if (response.at("browser").get<std::string>() == "firefox")
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
				response.at("browserPath") = p.string();
				break;
			}
		}
	}
	else if (response.at("browser").get<std::string>() == "edge")
	{
		// only for windows
		fs::path edgePath("C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe");
		edgePath.make_preferred();
		response.at("browserPath") = edgePath.string();
	}
	else if (response.at("browser").get<std::string>() == "safari")
	{
		// todo
		response.at("browserPath") = std::string("");
	}
	else if (response.at("browser").get<std::string>() == "default")
	{
		// do nothing
	}
	else
	{
		// N/A
		//   do nothing
	}

	////
	// update openAs
	if (parameters.contains("openAs") && !(parameters.at("openAs").get<std::string>().size() > 0))
	{
		response.at("openAs") = parameters.at("openAs").get<std::string>();
	}
	// update response["openAs"]
	if (response.at("browser").get<std::string>() == "chrome")
	{
		// chrome supports "app"|"window"|"tab"|"none"
		//   precisely speaking, "tab" and "none" are the same
	}
	else if (response.at("browser").get<std::string>() == "firefox")
	{
		// chrome supports "window"|"tab"|"none"
		//   precisely speaking, "tab" and "none" are the same
		if (response.at("openAs").get<std::string>() == "firefox")
		{
			response.at("openAs") = std::string("window");
		}
	}
	else if (response.at("browser").get<std::string>() == "edge")
	{
		// edge supports "app"|"window"|"tab"|"none"
		//   precisely speaking, "tab" and "none" are the same
	}
	else if (response.at("browser").get<std::string>() == "safari")
	{
		// todo check which mode is supported by safari ...
		// safari supports "app"|"window"|"tab"|"none"
		//   precisely speaking, "tab" and "none" are the same
	}
	else if (response.at("browser").get<std::string>() == "default")
	{
		// for default browser (start/open command), we cannot know which type is supported ...
		response.at("openAs") = std::string("none");
	}
	else
	{
		// for N/A (specified by path), we cannot know which which type is supported ...
		response.at("openAs") = std::string("none");
	}

	////
	// prepare cmd
	// todo
}

#endif
