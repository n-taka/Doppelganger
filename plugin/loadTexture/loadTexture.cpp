#ifndef LOADTEXTURE_CPP
#define LOADTEXTURE_CPP

#include "../pluginHeader.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/triangleMesh.h"
#include "Doppelganger/Logger.h"
#include "Util/uuid.h"
#include "Util/writeBase64ToFile.h"

#include <string>
#include <sstream>
#include <mutex>
#if defined(_WIN32) || defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "Util/stb_image.h"

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
	// 	"meshUUID": which mesh we try to add texture?
	// 	"texture": {
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
	// response = {
	// }
	// broadcast = {
	// 	"meshes" : {
	//    "<meshUUID>": JSON object that represents the loaded mesh
	//  }
	// }

	// create empty response/broadcast
	response = nlohmann::json::object();
	broadcast = nlohmann::json::object();

	const std::string &fileId = parameters.at("texture").at("file").at("id").get<std::string>();
	const int &fileSize = parameters.at("texture").at("file").at("size").get<int>();
	const int &packetId = parameters.at("texture").at("file").at("packetId").get<int>();
	const int &packetSize = parameters.at("texture").at("file").at("packetSize").get<int>();
	const int &packetTotal = parameters.at("texture").at("file").at("packetTotal").get<int>();
	const std::string &fileType = parameters.at("texture").at("file").at("type").get<std::string>();
	const std::string &base64Packet = parameters.at("texture").at("file").at("base64Packet").get<std::string>();

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
		std::lock_guard<std::mutex> lock(room->mutexMeshes);

		const std::string meshUUID = parameters.at("meshUUID").get<std::string>();
		const std::shared_ptr<Doppelganger::triangleMesh> &mesh = room->meshes.at(meshUUID);

		// edit history
		nlohmann::json diff = nlohmann::json::object();
		nlohmann::json diffInv = nlohmann::json::object();
		diffInv["meshes"] = nlohmann::json::object();
		diffInv["meshes"][meshUUID] = mesh->dumpToJson(false);
		diffInv["meshes"][meshUUID]["remove"] = false;

		fs::path filePath = fs::temp_directory_path();
		filePath /= Doppelganger::Util::uuid("DoppelgangerTmpFile-");
		filePath += ".";
		filePath += fileType;
		Doppelganger::Util::writeBase64ToFile(base64Str, filePath);

		Doppelganger::triangleMesh::Texture texture;
		{
			texture.fileName = parameters.at("texture").at("name").get<std::string>();

			int width, height, channels;
			// first pixel corresponds to the lower left corner
			stbi_set_flip_vertically_on_load(true);
			unsigned char *image = stbi_load(filePath.string().c_str(),
											 &width,
											 &height,
											 &channels,
											 STBI_rgb_alpha);

			texture.texData.resize(height, width);
			union
			{
				uint32_t uint_;
				unsigned char uchar_[4];
			} u;

			for (int h = 0; h < height; ++h)
			{
				for (int w = 0; w < width; ++w)
				{
					for (int rgba = 0; rgba < STBI_rgb_alpha; ++rgba)
					{
						u.uchar_[rgba] = image[(h * width + w) * STBI_rgb_alpha + rgba];
					}
					texture.texData(h, w) = u.uint_;
				}
			}
			stbi_image_free(image);
		}
		mesh->textures.push_back(texture);

		// write response/broadcast
		broadcast["meshes"] = nlohmann::json::object();
		broadcast.at("meshes")[meshUUID] = mesh->dumpToJson(true);

		{
			// message
			std::stringstream ss;
			ss << "New texture for \"" << meshUUID << "\" is loaded.";
			room->logger.log(ss.str(), "APICALL");
			// file
			room->logger.log(filePath, "APICALL", true);
		}

		// edit history
		diff["meshes"] = nlohmann::json::object();
		diff["meshes"][meshUUID] = mesh->dumpToJson(false);
		diff["meshes"][meshUUID]["remove"] = false;
		room->storeHistory(diff, diffInv);
	}
}

#endif
