#ifndef SHUTDOWN_CPP
#define SHUTDOWN_CPP

#include "../pluginHeader.h"
#include "nlohmann/json.hpp"
#include "Doppelganger/Core.h"
#include "Doppelganger/Room.h"

#include <string>

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
	// }
	// broadcast = {
	// }

	// create response/broadcast
	response = nlohmann::json::object();
	broadcast = nlohmann::json::object();

	// we remove log directory (graceful shutdown)
	if (room->core->config.contains("output"))
	{
		const nlohmann::json &outputJson = room->core->config.at("output");
		if (outputJson.contains("type") && outputJson.at("type").get<std::string>() == "local")
		{
			const nlohmann::json &outputLocalJson = outputJson.at("local");
			fs::remove_all(outputLocalJson.at("dir").get<std::string>());
		}
	}

	// graceful shutdown
	room->core->ioc.stop();
}

#endif
