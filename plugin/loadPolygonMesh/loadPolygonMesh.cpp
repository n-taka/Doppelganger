#ifndef LOADPOLYGONMESH_CPP
#define LOADPOLYGONMESH_CPP

#include "../pluginHeader.h"

#include <string>
#include <sstream>
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
	response["version"] = "1.0.0";
}

extern "C" DLLEXPORT void pluginProcess(const std::shared_ptr<Doppelganger::Room> &room, const nlohmann::json &parameters, nlohmann::json &response)
{
	////
	// [IN]
	//    parameters: config json
	// [OUT]
	//    response[""] =

	// writeBase64ToFile(room, parameters, response);
	// if (response.contains("tmpFileName"))
	{
		// const fs::path tmpFilePath(response.at("tmpFileName").get<std::string>());

		// std::vector<int> updateDoppelId;

		// const std::string meshName = parameters.at("meshName");

		// std::vector<Doppel::triangleMesh> newMeshes;
		// readMeshFromPolygonFile(
		// 	tmpFilePath,
		// 	meshName,
		// 	false,
		// 	newMeshes);

		// {
		// 	std::lock_guard<std::mutex> lock(pantry->mutexMeshes);
		// 	for (const auto &m : newMeshes)
		// 	{
		// 		int freshId = 0;
		// 		for (const auto &id_m : pantry->meshes)
		// 		{
		// 			freshId = std::max(freshId, id_m.first + 1);
		// 		}
		// 		pantry->meshes[freshId] = m;
		// 		updateDoppelId.push_back(freshId);
		// 	}

		// 	Doppel::Logger logger(pantry);
		// 	logger.logFile(tmpFilePath, Doppel::Logger::LogLevel::APICALL);
		// 	fs::remove(tmpFilePath);

		// 	Doppel::HistoryHandler handler;
		// 	handler.saveCurrentState(updateDoppelId, pantry);
		// 	pantry->broadcastMeshUpdate(updateDoppelId);
		// }
	}
}

#endif
