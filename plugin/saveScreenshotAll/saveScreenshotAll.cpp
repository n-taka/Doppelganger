#ifndef SAVESCREENSHOTALL_CPP
#define SAVESCREENSHOTALL_CPP

#include "../pluginHeader.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/Core.h"
#include "Doppelganger/Logger.h"
#include "Doppelganger/triangleMesh.h"
#include "Util/writeBase64ToFile.h"

#include <string>
#include <mutex>
#include <fstream>

#include <string>
#if defined(_WIN32) || defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#endif

extern "C" DLLEXPORT void metadata(const std::shared_ptr<Doppelganger::Room> &room, const nlohmann::json &parameters, nlohmann::json &response, nlohmann::json &broadcast)
{
	response["author"] = "Kazutaka Nakashima";
	response["version"] = "1.0.0";
	broadcast = nlohmann::json::object();
}

namespace
{
	std::unordered_map<std::string, std::pair<std::vector<bool>, std::string>> packetsVec;
	std::mutex mutexPackets;
}

extern "C" DLLEXPORT void pluginProcess(const std::shared_ptr<Doppelganger::Room> &room, const nlohmann::json &parameters, nlohmann::json &response, nlohmann::json &broadcast)
{
	////
	// [IN]
	// parameters = {
	// 	"screenshot": {
	// 	 "name": name of this mesh (usually, filename without extension),
	// 	 "file": {
	//    "id": unique id for this file,
	//    "size": bytes of this file,
	//    "packetId": id for this packet,
	//    "packetSize": size of each packet,
	//    "packetTotal": total count of packets,
	// 	  "type": extensiton of this file,
	// 	  "base64Packet": base64-encoded fragment
	// 	 }
	// 	}
	// }

	// [OUT]
	// <when core.config.at("output").at("type") == "download">
	// response = {
	// 	"screenshots" : [
	// 	 {
	//    "fileName": fileName,
	//    "base64Str": base64-encoded data,
	//    "format": "jpeg"|"png"
	//   },
	//   ...
	//  ]
	// }
	// <when core.config.at("output").at("type") == "local">
	// response = {
	// }
	// broadcast = {
	// }

	// create empty response/broadcast
	response = nlohmann::json::object();
	broadcast = nlohmann::json::object();

	const std::string &fileName = parameters.at("screenshot").at("name").get<std::string>();
	const std::string &fileId = parameters.at("screenshot").at("file").at("id").get<std::string>();
	const int &fileSize = parameters.at("screenshot").at("file").at("size").get<int>();
	const int &packetId = parameters.at("screenshot").at("file").at("packetId").get<int>();
	const int &packetSize = parameters.at("screenshot").at("file").at("packetSize").get<int>();
	const int &packetTotal = parameters.at("screenshot").at("file").at("packetTotal").get<int>();
	const std::string &fileType = parameters.at("screenshot").at("file").at("type").get<std::string>();
	const std::string &base64Packet = parameters.at("screenshot").at("file").at("base64Packet").get<std::string>();

	bool allPacketArrived = true;
	std::string base64Str;
	{
		std::lock_guard<std::mutex> lock(mutexPackets);

		std::pair<std::vector<bool>, std::string> &packet = packetsVec[fileId];
		std::vector<bool> &packetArrived = packet.first;
		std::string &packetBase64Str = packet.second;

		if (packetArrived.size() == 0)
		{
			// initialize.
			packetArrived.resize(packetTotal, false);
			packetBase64Str = std::string(fileSize, '\0');
		}

		packetArrived.at(packetId) = true;
		packetBase64Str.replace(packetId * packetSize, base64Packet.size(), base64Packet);

		for (const auto &b : packetArrived)
		{
			allPacketArrived &= b;
		}

		if (allPacketArrived)
		{
			base64Str = std::move(packetBase64Str);
		}
	}

	if (base64Str.size())
	{
		const bool saveToLocal = (room->core->config.at("output").at("type").get<std::string>() == "local");
		if (saveToLocal)
		{
			const fs::path directoryPath(room->outputDir);
			// path to screenshot file
			fs::path screenshotFilePath;
			{
				std::string screenshotFileName(fileName);
				screenshotFileName += ".";
				screenshotFileName += fileType;
				screenshotFilePath = directoryPath;
				screenshotFilePath.append(screenshotFileName);
			}
			Doppelganger::Util::writeBase64ToFile(base64Str, screenshotFilePath);

			// open a directory that containing filePath
			// open output directory.
			std::stringstream cmd;
#if defined(_WIN32) || defined(_WIN64)
			cmd << "start \"\" \"";
#elif defined(__APPLE__)
			cmd << "open \"";
#endif
			cmd << room->outputDir.string();
			cmd << "\"";
			system(cmd.str().c_str());
		}
		else
		{
			response["screenshots"] = nlohmann::json::array();
			nlohmann::json screenshotJson = nlohmann::json::object();
			screenshotJson["fileName"] = fileName;
			screenshotJson["base64Str"] = base64Str;
			screenshotJson["format"] = fileType;
			response["screenshots"].push_back(screenshotJson);
		}
	}
}

#endif
