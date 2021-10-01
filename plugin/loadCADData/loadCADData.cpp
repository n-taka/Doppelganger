#ifndef LOADCADDATA_CPP
#define LOADCADDATA_CPP

#include "../pluginHeader.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/Core.h"
#include "Doppelganger/Plugin.h"
#include "Doppelganger/Logger.h"
#include "Doppelganger/triangleMesh.h"
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

#include "igl/readPLY.h"
#include "igl/remove_duplicate_vertices.h"

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
	// 	"CAD": {
	// 	 "name": name of this CAD (usually, filename without extension),
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

	const std::string &CADFileName = parameters.at("CAD").at("name").get<std::string>();
	const std::string &fileId = parameters.at("CAD").at("file").at("id").get<std::string>();
	const int &fileSize = parameters.at("CAD").at("file").at("size").get<int>();
	const int &packetId = parameters.at("CAD").at("file").at("packetId").get<int>();
	const int &packetSize = parameters.at("CAD").at("file").at("packetSize").get<int>();
	const int &packetTotal = parameters.at("CAD").at("file").at("packetTotal").get<int>();
	const std::string &fileType = parameters.at("CAD").at("file").at("type").get<std::string>();
	const std::string &base64Packet = parameters.at("CAD").at("file").at("base64Packet").get<std::string>();

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
		fs::path filePath = fs::temp_directory_path();
		filePath /= Doppelganger::Util::uuid("DoppelgangerTmpFile-");
		filePath += ".";
		filePath += fileType;
		Doppelganger::Util::writeBase64ToFile(base64Str, filePath);

		// find FreeCADCmd binary
		fs::path FreeCADCmd;
		{
#if defined(_WIN32) || defined(_WIN64)
			std::vector<fs::path> ProgramFilesPaths(
				{fs::path("C:\\Program Files"),
				 fs::path("C:\\Program Files (x86)")});
			const std::string FreeCAD("FreeCAD");
			for (const auto &ProgramFiles : ProgramFilesPaths)
			{
				if (!FreeCADCmd.empty())
				{
					break;
				}
				for (const auto &p : fs::directory_iterator(ProgramFiles))
				{
					if (!FreeCADCmd.empty())
					{
						break;
					}
					const std::string &programName = p.path().filename().string();
					const auto res = std::mismatch(FreeCAD.begin(), FreeCAD.end(), programName.begin());
					if (res.first == FreeCAD.end())
					{
						FreeCADCmd = p;
						FreeCADCmd.append("bin");
						FreeCADCmd.append("FreeCADCmd.exe");
					}
				}
			}
#elif defined(__APPLE__)
			// todo
			FreeCADCmd = fs::path("/Applications/FreeCAD.app/Contents/Resources/bin/FreeCADCmd");
#endif
			FreeCADCmd.make_preferred();
		}

		if (!FreeCADCmd.empty())
		{
			// find script
			fs::path FreeCADScript;
			{
				const fs::path pluginsDir(room->core->config.at("plugin").at("dir").get<std::string>());
				fs::path pluginDir(pluginsDir);
				std::string dirName("loadCADData");
				dirName += "_";
				std::string installedVersion(room->core->plugin.at("loadCADData")->installedVersion);
				if (installedVersion == "latest")
				{
					installedVersion = room->core->plugin.at("loadCADData")->parameters.at("latest").get<std::string>();
				}
				dirName += installedVersion;
				pluginDir.append(dirName);

				FreeCADScript = fs::path(pluginDir);
				FreeCADScript.append("FreeCADConvert.py");
			}

			// export FreeCADConfig.json
			fs::path FreeCADConfig;
			{
				nlohmann::json json = nlohmann::json::object();
				json["filePath"] = filePath.string();
				json["meshName"] = CADFileName;
				json["merge"] = false;
				json["maxTolerance"] = 0.1;

				FreeCADConfig = fs::path(FreeCADScript);
				FreeCADConfig = FreeCADConfig.parent_path();
				FreeCADConfig.append("FreeCADConfig.json");
				std::ofstream ofs(FreeCADConfig);
				ofs << json.dump() << std::endl;
				ofs.close();
			}

			std::stringstream cmd;
#if defined(_WIN32) || defined(_WIN64)
			cmd << "\"" << FreeCADCmd << " " << FreeCADScript << "\"";
#elif defined(__APPLE__)
			cmd << FreeCADCmd << " " << FreeCADScript;
#endif

			std::cout << cmd.str() << std::endl;
			system(cmd.str().c_str());

			fs::path dirName(filePath);
			dirName += ".dump";

			std::vector<std::shared_ptr<Doppelganger::triangleMesh>> meshes;
			for (const auto &p : fs::directory_iterator(dirName))
			{
				if (p.path().extension() == ".ply")
				{
					const std::string meshUUID = Doppelganger::Util::uuid("mesh-");
					std::shared_ptr<Doppelganger::triangleMesh> mesh = std::make_shared<Doppelganger::triangleMesh>(meshUUID);
					meshes.push_back(mesh);

					mesh->name = p.path().stem().string();
					Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> temp_E;
					std::vector<std::string> Vheader({"red", "green", "blue"});
					igl::readPLY(p.path().string(), mesh->V, mesh->F, temp_E, mesh->VN, mesh->TC, mesh->VC, Vheader);
					// normalize vertex colors
					mesh->VC /= 255.;

					// log
					{
						std::stringstream ss;
						ss << "New mesh \"" << mesh->UUID << "\" is loaded.";
						room->logger.log(ss.str(), "APICALL");
						// file
						room->logger.log(p.path(), "APICALL", true);
					}
				}
			}

			nlohmann::json diff = nlohmann::json::object();
			nlohmann::json diffInv = nlohmann::json::object();
			broadcast["meshes"] = nlohmann::json::object();
			diff["meshes"] = nlohmann::json::object();
			diffInv["meshes"] = nlohmann::json::object();
			for (const auto &mesh : meshes)
			{
				// for CAD mesh, we try to merge duplicate vertices
				// for CAD data, there are no vertex/face attributes
				Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> tempV = mesh->V;
				Eigen::Matrix<int, Eigen::Dynamic, 1> SVI, SVJ;
				igl::remove_duplicate_vertices(tempV, 0, mesh->V, SVI, SVJ);
				std::for_each(mesh->F.data(), mesh->F.data() + mesh->F.size(), [&SVJ](int &f)
							  { f = SVJ(f); });

				// register to room / edit history
				{
					room->meshes[mesh->UUID] = mesh;
					broadcast.at("meshes")[mesh->UUID] = mesh->dumpToJson(true);
					diff["meshes"][mesh->UUID] = mesh->dumpToJson(false);
					diff["meshes"][mesh->UUID]["remove"] = false;
					diffInv["meshes"][mesh->UUID] = nlohmann::json::object();
					diffInv["meshes"][mesh->UUID]["remove"] = true;
				}
			}

			fs::remove_all(dirName);
			fs::remove_all(FreeCADConfig);
		}
	}
}

#endif
