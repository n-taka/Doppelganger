#ifndef REPAIRMESH_CPP
#define REPAIRMESH_CPP

#include "../pluginHeader.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/Core.h"
#include "Doppelganger/Plugin.h"
#include "Doppelganger/Logger.h"
#include "Doppelganger/triangleMesh.h"
#include "Util/uuid.h"

#include <string>
#include <mutex>

#include <string>
#if defined(_WIN32) || defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#endif

#include "igl/facet_components.h"
#include "igl/remove_unreferenced.h"
#include "igl/writePLY.h"
#include "igl/readOFF.h"

extern "C" DLLEXPORT void metadata(const std::shared_ptr<Doppelganger::Room> &room, const nlohmann::json &parameters, nlohmann::json &response, nlohmann::json &broadcast)
{
	response["author"] = "Kazutaka Nakashima";
	response["version"] = "1.0.0";
	broadcast = nlohmann::json::object();
}

extern "C" DLLEXPORT void pluginProcess(const std::shared_ptr<Doppelganger::Room> &room, const nlohmann::json &parameters, nlohmann::json &response, nlohmann::json &broadcast)
{
	////
	// [IN]
	// parameters = {
	// 	"meshes": [
	// 	 "UUID-A",
	// 	 "UUID-B",
	//   ...
	//  ]
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
	nlohmann::json diff = nlohmann::json::object();
	nlohmann::json diffInv = nlohmann::json::object();

	broadcast["meshes"] = nlohmann::json::object();
	diff["meshes"] = nlohmann::json::object();
	diffInv["meshes"] = nlohmann::json::object();
	{
		std::lock_guard<std::mutex> lock(room->mutexMeshes);

		// remove mesh
		for (const auto &UUID : parameters.at("meshes"))
		{
			const std::string &meshUUID = UUID.get<std::string>();
			// we copy pointer (because we call room->meshes.erase(meshUUID) later)
			const std::shared_ptr<Doppelganger::triangleMesh> mesh = room->meshes.at(meshUUID);
			// store current mesh
			// current mesh is removed and new mesh is constructed for each connected component
			diffInv.at("meshes")[meshUUID] = mesh->dumpToJson(false);
			diffInv.at("meshes").at(meshUUID)["remove"] = false;
			// remove mesh (from here, shared_ptr "mesh" is invalidated.)
			room->meshes.erase(meshUUID);
			broadcast.at("meshes")[meshUUID] = nlohmann::json::object();
			broadcast.at("meshes").at(meshUUID)["remove"] = true;
			diff.at("meshes")[meshUUID] = nlohmann::json::object();
			diff.at("meshes").at(meshUUID)["remove"] = true;
			// introduce repaired mesh
			//   content of this mesh is updated later
			const std::string repairedUUID = Doppelganger::Util::uuid("mesh-");
			std::shared_ptr<Doppelganger::triangleMesh> repairedMesh = std::make_shared<Doppelganger::triangleMesh>(repairedUUID);
			repairedMesh->name = mesh->name;
			repairedMesh->name += "_repaired";
			room->meshes[repairedUUID] = repairedMesh;

			//// repair
			// find binary executable
			fs::path meshFix;
			{
				const fs::path pluginsDir(room->core->config.at("plugin").at("dir").get<std::string>());
				fs::path pluginDir(pluginsDir);
				std::string dirName("repairMesh");
				dirName += "_";
				std::string installedVersion(room->core->plugin.at("repairMesh")->installedVersion);
				if (installedVersion == "latest")
				{
					installedVersion = room->core->plugin.at("repairMesh")->parameters.at("latest").get<std::string>();
				}
				dirName += installedVersion;
				pluginDir.append(dirName);

				meshFix = fs::path(pluginDir);
#if defined(_WIN32) || defined(_WIN64)
				meshFix.append("MeshFix.exe");
#else
				meshFix.append("MeshFix");
#endif
			}

			// Because MeshFix only fixes the largest component
			//   (small components are removed)
			// So, we apply MeshFix for each connected components,
			// and then merge all the fixed meshes.
			// **NOTE** we have asssumption that the target mesh is
			// "well-connected"
			Eigen::Matrix<int, Eigen::Dynamic, 1> C;
			igl::facet_components(mesh->F, C);
			repairedMesh->V.resize(0, mesh->V.cols());
			repairedMesh->F.resize(0, mesh->F.cols());

			for (int c = 0; c < C.maxCoeff() + 1; ++c)
			{
				Doppelganger::triangleMesh componentMesh;
				Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> componentV;
				Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> componentF;

				const int componentFCount = (C.array() == c).count();
				componentF.resize(componentFCount, mesh->F.cols());
				int fIdx = 0;
				for (int f = 0; f < C.rows(); ++f)
				{
					if (C(f) == c)
					{
						componentF.row(fIdx++) = mesh->F.row(f);
					}
				}
				Eigen::Matrix<int, Eigen::Dynamic, 1> I;
				igl::remove_unreferenced(mesh->V, componentF, componentMesh.V, componentMesh.F, I);

				fs::path fileNameIn, fileNameOut;
				const std::string tmpFileName = Doppelganger::Util::uuid("DoppelgangerTmpFile-");
				fileNameIn = fs::temp_directory_path();
				fileNameOut = fs::temp_directory_path();
				fileNameIn /= tmpFileName;
				fileNameOut /= tmpFileName;
				fileNameIn += ".ply";
				fileNameOut += "_fixed.off";

				// because we use projectMeshAttributes later, we only care about the geometry here.
				// MeshFix requires float (not double) for floating-point
				igl::writePLY(fileNameIn.string(), componentMesh.V.template cast<float>(), componentMesh.F);

				std::stringstream cmd;
				cmd << "\"";
				cmd << meshFix;
				cmd << " ";
				cmd << fileNameIn;
				cmd << " ";
				cmd << fileNameOut;
				cmd << "\"";
				room->logger.log(cmd.str(), "APICALL");
				system(cmd.str().c_str());

				igl::readOFF(fileNameOut.string(), componentV, componentF);
				if (componentF.rows() > 0)
				{
					Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> tempV = std::move(repairedMesh->V);
					Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> tempF = std::move(repairedMesh->F);
					repairedMesh->V.resize(tempV.rows() + componentV.rows(), tempV.cols());
					repairedMesh->F.resize(tempF.rows() + componentF.rows(), tempF.cols());
					repairedMesh->V << tempV,
						componentV;
					repairedMesh->F << tempF,
						(componentF.array() + tempV.rows()).matrix();
				}

				fs::remove(fileNameIn);
				fs::remove(fileNameOut);
			}

			repairedMesh->projectMeshAttributes(mesh);

			// update diff, broadcast
			broadcast["meshes"][repairedUUID] = repairedMesh->dumpToJson(true);
			broadcast["meshes"][repairedUUID]["remove"] = false;
			diff["meshes"][repairedUUID] = repairedMesh->dumpToJson(false);
			diff["meshes"][repairedUUID]["remove"] = false;
			diffInv["meshes"][repairedUUID] = nlohmann::json::object();
			diffInv["meshes"][repairedUUID]["remove"] = true;
		}
	}
	room->storeHistory(diff, diffInv);
}

#endif
