#ifndef SPLITINTOISLANDS_CPP
#define SPLITINTOISLANDS_CPP

#include "../pluginHeader.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/triangleMesh.h"
#include "Util/uuid.h"

#include <string>
#include <mutex>
#include <vector>
#include <unordered_map>

#include "igl/facet_components.h"
#include "igl/remove_unreferenced.h"

#include "igl/per_face_normals.h"
#include "igl/per_vertex_normals.h"

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
			room->meshes.erase(meshUUID);
			broadcast.at("meshes")[meshUUID] = nlohmann::json::object();
			broadcast.at("meshes").at(meshUUID)["remove"] = true;
			diff.at("meshes")[meshUUID] = nlohmann::json::object();
			diff.at("meshes").at(meshUUID)["remove"] = true;

			Eigen::Matrix<int, Eigen::Dynamic, 1> C;
			igl::facet_components(mesh->F, C);
			std::unordered_map<int, std::vector<int>> faceIdVec;
			for (int f = 0; f < C.rows(); ++f)
			{
				faceIdVec[C(f, 0)].push_back(f);
			}
			for (const auto &id_faces : faceIdVec)
			{
				const int &id = id_faces.first;
				const std::vector<int> &faces = id_faces.second;

				Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> tmpF;
				tmpF.resize(faces.size(), mesh->F.cols());
				for (int ff = 0; ff < faces.size(); ++ff)
				{
					tmpF.row(ff) = mesh->F.row(faces.at(ff));
				}
				const std::string componentUUID = Doppelganger::Util::uuid("mesh-");
				std::shared_ptr<Doppelganger::triangleMesh> componentMesh = std::make_shared<Doppelganger::triangleMesh>(componentUUID);
				componentMesh->name = mesh->name;
				componentMesh->name += "_";
				componentMesh->name += std::to_string(id);
				Eigen::Matrix<int, Eigen::Dynamic, 1> I;
				igl::remove_unreferenced(mesh->V, tmpF, componentMesh->V, componentMesh->F, I);

				// project attributes
				// for better performance, we should NOT use this and use result of remove_unreferenced (i.e. Matrix I, J)
				//   (projectMeshAttributes uses signed distance and its slow...)
				componentMesh->projectMeshAttirbutes(mesh);

				room->meshes[componentUUID] = componentMesh;
				broadcast["meshes"][componentUUID] = componentMesh->dumpToJson(true);
				broadcast["meshes"][componentUUID]["remove"] = false;
				diff["meshes"][componentUUID] = componentMesh->dumpToJson(false);
				diff["meshes"][componentUUID]["remove"] = false;
				diffInv["meshes"][componentUUID] = nlohmann::json::object();
				diffInv["meshes"][componentUUID]["remove"] = true;
			}
		}
	}
	room->storeHistory(diff, diffInv);
}

#endif
