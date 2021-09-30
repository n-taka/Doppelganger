#ifndef MESHERRORINFO_CPP
#define MESHERRORINFO_CPP

#include "../pluginHeader.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/triangleMesh.h"

#include "igl/boundary_facets.h"
#include "igl/is_edge_manifold.h"
#include "igl/is_vertex_manifold.h"

#include <string>
#include <mutex>

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
	// 	"meshes" : {
	//   "<meshUUID>": {
	// 	  "closed" : boolean flag
	// 	  "edgeManifold" : boolean flag
	// 	  "vertexManifold" : boolean flag
	//   },
	//   ...
	//  }
	// }
	// broadcast = {
	// }

	// create empty response/broadcast
	response = nlohmann::json::object();
	broadcast = nlohmann::json::object();

	response["meshes"] = nlohmann::json::object();
	{
		std::lock_guard<std::mutex> lock(room->mutexMeshes);

		for (const auto &UUID : parameters.at("meshes"))
		{
			const std::string &meshUUID = UUID.get<std::string>();
			const std::shared_ptr<Doppelganger::triangleMesh> &mesh = room->meshes.at(meshUUID);

			nlohmann::json errorInfo = nlohmann::json::object();
			{
				Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic> BF;
				igl::boundary_facets(mesh->F, BF);
				errorInfo["closed"] = (BF.rows() == 0);
			}
			{
				errorInfo["edgeManifold"] = igl::is_edge_manifold(mesh->F);
			}
			{
				Eigen::Matrix<int, Eigen::Dynamic, 1> B;
				errorInfo["vertexManifold"] = igl::is_vertex_manifold(mesh->F, B);
			}

			response.at("meshes")[meshUUID] = errorInfo;
		}
	}
}

#endif
