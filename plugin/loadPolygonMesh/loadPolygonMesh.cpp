#ifndef LOADPOLYGONMESH_CPP
#define LOADPOLYGONMESH_CPP

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

#include "igl/readOBJ.h"
#include "igl/readPLY.h"
#include "igl/readSTL.h"
#include "igl/readWRL.h"
#include "igl/remove_duplicate_vertices.h"
#include "igl/polygon_corners.h"
#include "igl/polygons_to_triangles.h"

#include "igl/per_face_normals.h"
#include "igl/per_vertex_normals.h"

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
	// 	"mesh": {
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

	const std::string &fileId = parameters.at("mesh").at("file").at("id").get<std::string>();
	const int &fileSize = parameters.at("mesh").at("file").at("size").get<int>();
	const int &packetId = parameters.at("mesh").at("file").at("packetId").get<int>();
	const int &packetSize = parameters.at("mesh").at("file").at("packetSize").get<int>();
	const int &packetTotal = parameters.at("mesh").at("file").at("packetTotal").get<int>();
	const std::string &fileType = parameters.at("mesh").at("file").at("type").get<std::string>();
	const std::string &base64Packet = parameters.at("mesh").at("file").at("base64Packet").get<std::string>();

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

		const std::string meshUUID = Doppelganger::Util::uuid("mesh-");
		std::shared_ptr<Doppelganger::triangleMesh> mesh = std::make_shared<Doppelganger::triangleMesh>(meshUUID);

		const std::string &meshName = parameters.at("mesh").at("name").get<std::string>();
		mesh->name = meshName;
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> &V = mesh->V;
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> &VN = mesh->VN;
		Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> &F = mesh->F;
		Eigen::Matrix<int, Eigen::Dynamic, 1> &FG = mesh->FG;
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> &FN = mesh->FN;
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> &VC = mesh->VC;
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> &TC = mesh->TC;
		Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> &FTC = mesh->FTC;

		std::vector<std::vector<double>> VVec;
		std::vector<std::vector<double>> TCVec;
		std::vector<std::vector<double>> VNVec;
		std::vector<std::vector<double>> VCVec;
		std::vector<std::vector<int>> FVec;
		std::vector<std::vector<int>> FTCVec;
		std::vector<std::vector<int>> FNVec;
		std::vector<int> FGVec;

		if (fileType == "obj")
		{
			std::vector<std::tuple<std::string, int, int>> FM;
			igl::readOBJ(filePath.string(), VVec, TCVec, VNVec, VCVec, FVec, FTCVec, FNVec, FGVec, FM);
		}
		else if (fileType == "ply")
		{
			Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> temp_V, temp_E, temp_VN, temp_VC, temp_TC;
			Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> temp_F;
			std::vector<std::string> Vheader({"red", "green", "blue"});
			igl::readPLY(filePath.string(), temp_V, temp_F, temp_E, temp_VN, temp_TC, temp_VC, Vheader);
			// normalize vertex colors
			temp_VC /= 255.;

			// write into vector (this is not optimal, but makes the code easy-to-read)
			VVec.resize(temp_V.rows());
			for (int v = 0; v < temp_V.rows(); ++v)
			{
				VVec.at(v) = std::vector<double>({temp_V(v, 0), temp_V(v, 1), temp_V(v, 2)});
			}
			FVec.resize(temp_F.rows());
			for (int f = 0; f < temp_F.rows(); ++f)
			{
				FVec.at(f) = std::vector<int>({temp_F(f, 0), temp_F(f, 1), temp_F(f, 2)});
			}
			VNVec.resize(temp_VN.rows());
			for (int v = 0; v < temp_VN.rows(); ++v)
			{
				VNVec.at(v) = std::vector<double>({temp_VN(v, 0), temp_VN(v, 1), temp_VN(v, 2)});
			}
			TCVec.resize(temp_TC.rows());
			for (int v = 0; v < temp_TC.rows(); ++v)
			{
				TCVec.at(v) = std::vector<double>({temp_TC(v, 0), temp_TC(v, 1)});
			}
			VCVec.resize(temp_VC.rows());
			for (int v = 0; v < temp_VC.rows(); ++v)
			{
				VCVec.at(v) = std::vector<double>({temp_VC(v, 0), temp_VC(v, 1), temp_VC(v, 2)});
			}
		}
		else if (fileType == "stl")
		{
			// todo: support vertex color (magics extension)
			Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> temp_V, unified_V, temp_FN;
			Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> temp_F;
			Eigen::Matrix<int, Eigen::Dynamic, 1> SVI, SVJ;
			FILE *fp = fopen(filePath.string().c_str(), "rb");
			igl::readSTL(fp, temp_V, temp_F, temp_FN);
			fclose(fp);
			igl::remove_duplicate_vertices(temp_V, 0, unified_V, SVI, SVJ);
			std::for_each(temp_F.data(), temp_F.data() + temp_F.size(), [&SVJ](int &f)
						  { f = SVJ(f); });

			// write into vector (this is not optimal, but makes the code easy-to-read)
			VVec.resize(unified_V.rows());
			for (int v = 0; v < unified_V.rows(); ++v)
			{
				VVec.at(v) = std::vector<double>({unified_V(v, 0), unified_V(v, 1), unified_V(v, 2)});
			}
			FVec.resize(temp_F.rows());
			for (int f = 0; f < temp_F.rows(); ++f)
			{
				FVec.at(f) = std::vector<int>({temp_F(f, 0), temp_F(f, 1), temp_F(f, 2)});
			}
			// Currently, FN is ignored.
		}
		else if (fileType == "wrl")
		{
			igl::readWRL(filePath.string(), VVec, FVec, VCVec);
		}
		else
		{
			std::stringstream ss;
			ss << "file type ";
			ss << fileType;
			ss << " is not yet supported.";
			room->logger.log(ss.str(), "ERROR");
		}

		// format parsed info
		// V (required)
		{
			V.resize(VVec.size(), 3);
			for (int v = 0; v < VVec.size(); ++v)
			{
				V.row(v) << VVec.at(v).at(0), VVec.at(v).at(1), VVec.at(v).at(2);
			}
		}
		// F (required)
		{
			Eigen::Matrix<int, Eigen::Dynamic, 1> I, C;
			igl::polygon_corners(FVec, I, C);
			Eigen::Matrix<int, Eigen::Dynamic, 1> J;
			igl::polygons_to_triangles(I, C, F, J);
		}

		// VN (optional, calculate manually)
		VN.resize(VNVec.size(), 3);
		for (int v = 0; v < VNVec.size(); ++v)
		{
			VN.row(v) << VNVec.at(v).at(0), VNVec.at(v).at(1), VNVec.at(v).at(2);
		}
		if (VVec.size() != VNVec.size())
		{
			igl::per_vertex_normals(V, F, VN);
		}
		// FN (optional)
		igl::per_face_normals(V, F, FN);
		// VC (optional, but filled manually)
		VC.resize(VCVec.size(), 3);
		for (int v = 0; v < VCVec.size(); ++v)
		{
			VC.row(v) << VCVec.at(v).at(0), VCVec.at(v).at(1), VCVec.at(v).at(2);
		}
		// TC (optional)
		TC.resize(TCVec.size(), 2);
		for (int v = 0; v < TCVec.size(); ++v)
		{
			TC.row(v) << TCVec.at(v).at(0), TCVec.at(v).at(1);
		}
		// FTC (optional)
		{
			Eigen::Matrix<int, Eigen::Dynamic, 1> I, C;
			igl::polygon_corners(FTCVec, I, C);
			Eigen::Matrix<int, Eigen::Dynamic, 1> J;
			igl::polygons_to_triangles(I, C, FTC, J);
		}
		// FG (optional)
		// for dealing with non-triangle polygons (which is triangulated onLoad), we manually fill FG
		if (FVec.size() == FGVec.size())
		{
			FG.resize(F.rows(), 1);
			int fIdx = 0;
			for (int f = 0; f < FVec.size(); ++f)
			{
				for (int tri = 0; tri < FVec.at(f).size() - 2; ++tri)
				{
					FG(fIdx++, 0) = FGVec.at(f);
				}
			}
		}

		// register to this room
		room->meshes[meshUUID] = mesh;

		// write response/broadcast
		broadcast["meshes"] = nlohmann::json::object();
		broadcast.at("meshes")[meshUUID] = mesh->dumpToJson(true);

		{
			// message
			std::stringstream ss;
			ss << "New mesh \"" << meshUUID << "\" is loaded.";
			room->logger.log(ss.str(), "APICALL");
			// file
			room->logger.log(filePath, "APICALL", true);
		}

		{
			// edit history
			nlohmann::json diff = nlohmann::json::object();
			nlohmann::json diffInv = nlohmann::json::object();
			diff["meshes"] = nlohmann::json::object();
			diff["meshes"][meshUUID] = mesh->dumpToJson(false);
			diff["meshes"][meshUUID]["remove"] = false;
			diffInv["meshes"] = nlohmann::json::object();
			diffInv["meshes"][meshUUID] = nlohmann::json::object();
			diffInv["meshes"][meshUUID]["remove"] = true;
			room->storeHistory(diff, diffInv);
		}
	}
}

#endif
