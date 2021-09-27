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
	//  }
	// }
	// * each timestamp are the same values
	// *   this is redundant, but consistent to API "pullCanvasParameters"

	// [OUT]
	// response = {
	// }
	// broadcast = parameters
	//  "controls": {
	//   "target": {
	//    "value": {
	//     "x": x-coordinate,
	//     "y": y-coordinate,
	//     "z": z-coordinate
	//    },
	//    "timestamp": timestamp
	//   },
	//  },
	//  "camera": {
	//   "position": {
	//    "value": {
	//     "x": x-coordinate,
	//     "y": y-coordinate,
	//     "z": z-coordinate
	//    },
	//    "timestamp": timestamp
	//   },
	//   "up": {
	//    "value": {
	//     "x": x-coordinate,
	//     "y": y-coordinate,
	//     "z": z-coordinate
	//    },
	//    "timestamp": timestamp
	//   },
	//   "zoom": {
	//    "value": zoom parameter,
	//    "timestamp": timestamp
	//   }
	//  }
	// }
	// * only with valid ones (i.e. if timestamp is older than the timestamp on the server, we filter such entry)

	// create response/broadcast
	response = nlohmann::json::object();
	broadcast = nlohmann::json::object();

	// controls
	if (parameters.contains("controls"))
	{
		nlohmann::json controlsBroadcast = nlohmann::json::object();

		const nlohmann::json &controls = parameters.at("controls");
		if (controls.contains("target"))
		{
			const nlohmann::json &target = controls.at("target");
			const std::int64_t &timestamp = target.at("timestamp").get<std::int64_t>();
			if (room->interfaceParams.cameraTargetTimestamp <= timestamp)
			{
				controlsBroadcast["target"] = target;
				room->interfaceParams.cameraTargetTimestamp = timestamp;
				room->interfaceParams.cameraTarget.x() = target.at("value").at("x").get<double>();
				room->interfaceParams.cameraTarget.y() = target.at("value").at("y").get<double>();
				room->interfaceParams.cameraTarget.z() = target.at("value").at("z").get<double>();
			}
		}
		if(!controlsBroadcast.empty())
		{
			broadcast["controls"] = controlsBroadcast;
		}
	}

	// camera
	if (parameters.contains("camera"))
	{
		nlohmann::json cameraBroadcast = nlohmann::json::object();

		const nlohmann::json &camera = parameters.at("camera");
		if (camera.contains("position"))
		{
			const nlohmann::json &position = camera.at("position");
			const std::int64_t &timestamp = position.at("timestamp").get<std::int64_t>();
			if (room->interfaceParams.cameraPositionTimestamp <= timestamp)
			{
				room->interfaceParams.cameraPositionTimestamp = timestamp;
				room->interfaceParams.cameraPosition.x() = position.at("value").at("x").get<double>();
				room->interfaceParams.cameraPosition.y() = position.at("value").at("y").get<double>();
				room->interfaceParams.cameraPosition.z() = position.at("value").at("z").get<double>();
				cameraBroadcast["position"] = position;
			}
		}
		if (camera.contains("up"))
		{
			const nlohmann::json &up = camera.at("up");
			const std::int64_t &timestamp = up.at("timestamp").get<std::int64_t>();
			if (room->interfaceParams.cameraUpTimestamp <= timestamp)
			{
				room->interfaceParams.cameraUpTimestamp = timestamp;
				room->interfaceParams.cameraUp.x() = up.at("value").at("x").get<double>();
				room->interfaceParams.cameraUp.y() = up.at("value").at("y").get<double>();
				room->interfaceParams.cameraUp.z() = up.at("value").at("z").get<double>();
				cameraBroadcast["up"] = up;
			}
		}
		if (camera.contains("zoom"))
		{
			const nlohmann::json &zoom = camera.at("zoom");
			const std::int64_t &timestamp = zoom.at("timestamp").get<std::int64_t>();
			if (room->interfaceParams.cameraZoomTimestamp <= timestamp)
			{
				room->interfaceParams.cameraZoomTimestamp = timestamp;
				room->interfaceParams.cameraZoom = zoom.at("value").get<double>();
				cameraBroadcast["zoom"] = zoom;
			}
		}
		if(!cameraBroadcast.empty())
		{
			broadcast["camera"] = cameraBroadcast;
		}
	}
}

#endif
