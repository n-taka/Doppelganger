#ifndef PULLCANVASPARAMETERS_CPP
#define PULLCANVASPARAMETERS_CPP

#include "../pluginHeader.h"
#include "nlohmann/json.hpp"
#include "Doppelganger/Room.h"
#include "Doppelganger/Plugin.h"

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
	//  "controls": {
	//   "target": {
	//    "x": x-coordinate,
	//    "y": y-coordinate,
	//    "z": z-coordinate,
	//    "timestamp": timestamp
	//   },
	//  },
	//  "camera": {
	//   "position": {
	//    "x": x-coordinate,
	//    "y": y-coordinate,
	//    "z": z-coordinate,
	//    "timestamp": timestamp
	//   },
	//   "up": {
	//    "x": x-coordinate,
	//    "y": y-coordinate,
	//    "z": z-coordinate,
	//    "timestamp": timestamp
	//   },
	//   "zoom": {
	//    "value": zoom parameter,
	//    "timestamp": timestamp
	//   }
	//  },
	//  "cursors": {
	//   "sessionUUID-A": {
	//    "dir": {
	//     "x": x-coordinate,
	//     "y": y-coordinate
	//    },
	//    "idx": idx for cursor icon
	//   },
	//   "sessionUUID-B": {
	//    "dir": {
	//     "x": x-coordinate,
	//     "y": y-coordinate
	//    },
	//    "idx": idx for cursor icon
	//   },
	//   ....
	//  }
	// }
	// broadcast = {
	// }

	// create response/broadcast
	response = nlohmann::json::object();
	broadcast = nlohmann::json::object();

	{
		nlohmann::json controls = nlohmann::json::object();
		{
			nlohmann::json target = nlohmann::json::object();
			target["x"] = room->interfaceParams.cameraTarget.x();
			target["y"] = room->interfaceParams.cameraTarget.y();
			target["z"] = room->interfaceParams.cameraTarget.z();
			target["timestamp"] = room->interfaceParams.cameraTargetTimestamp;
			controls["target"] = target;
		}
		response["controls"] = controls;
	}
	{
		nlohmann::json camera = nlohmann::json::object();
		{
			nlohmann::json position = nlohmann::json::object();
			position["x"] = room->interfaceParams.cameraPosition.x();
			position["y"] = room->interfaceParams.cameraPosition.y();
			position["z"] = room->interfaceParams.cameraPosition.z();
			position["timestamp"] = room->interfaceParams.cameraPositionTimestamp;
			camera["position"] = position;
		}
		{
			nlohmann::json up = nlohmann::json::object();
			up["x"] = room->interfaceParams.cameraUp.x();
			up["y"] = room->interfaceParams.cameraUp.y();
			up["z"] = room->interfaceParams.cameraUp.z();
			up["timestamp"] = room->interfaceParams.cameraUpTimestamp;
			camera["up"] = up;
		}
		{
			nlohmann::json zoom = nlohmann::json::object();
			zoom["value"] = room->interfaceParams.cameraZoom;
			zoom["timestamp"] = room->interfaceParams.cameraZoomTimestamp;
			camera["zoom"] = zoom;
		}
		response["camera"] = camera;
	}
	{
		nlohmann::json cursors = nlohmann::json::object();
		for (const auto &uuid_cursorInfo : room->interfaceParams.cursors)
		{
			const std::string &sessionUUID = uuid_cursorInfo.first;
			const Doppelganger::Room::cursorInfo &cursorInfo = uuid_cursorInfo.second;
			nlohmann::json cursorJson = nlohmann::json::object();
			{
				nlohmann::json dir = nlohmann::json::object();
				dir["x"] = cursorInfo.x;
				dir["y"] = cursorInfo.y;
				cursorJson["dir"] = dir;
			}
			cursorJson["idx"] = cursorInfo.idx;

			cursors[sessionUUID] = cursorJson;
		}

		response["cursors"] = cursors;
	}
}

#endif
