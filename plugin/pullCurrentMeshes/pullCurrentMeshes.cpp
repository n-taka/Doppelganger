#ifndef PULLCURRENTMESHES_CPP
#define PULLCURRENTMESHES_CPP

#include "../pluginHeader.h"
#include "Doppelganger/Room.h"
#include "Doppelganger/triangleMesh.h"

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
	// }

	// [OUT]
	// response = {
	// 	"meshes" : {
	//    "<meshUUID-A>": JSON object that represents the loaded mesh,
	//    "<meshUUID-B>": JSON object that represents the loaded mesh,
	//    ...
	//  }
	// }
	// broadcast = {
	// }

	// create empty response/broadcast
	response = nlohmann::json::object();
	broadcast = nlohmann::json::object();

	// write response/broadcast
	response["meshes"] = nlohmann::json::object();
	{
		std::lock_guard<std::mutex> lock(room->mutexMeshes);
		for (const auto &uuid_mesh : room->meshes)
		{
			const std::string &meshUUID = uuid_mesh.first;
			const std::shared_ptr<Doppelganger::triangleMesh> &mesh = uuid_mesh.second;
			response.at("meshes")[meshUUID] = mesh->dumpToJson(true);
		}
	}
}

#endif
