#ifndef ROOM_CPP
#define ROOM_CPP

#include "Room.h"
#include "Core.h"

// #include "util/loadTexture.h"
// #include "util/projectMeshAttributes.h"
// #include "util/readWriteMeshFromToFile.h"
// #include "util/saveTexture.h"
// #include "util/updateMeshesFromJson.h"
// #include "util/writeBase64ToFile.h"
// #include "util/writeMeshToJson.h"
// #include "util/writeMeshToMemory.h"

namespace Doppelganger
{
	Room::Room(const std::string &UUID_,
			   const std::shared_ptr<Core> &core_)
		: UUID(UUID_), core(core_)
	{
		//////
		// initialize server parameter
		//////
		{
			serverParams.mutexServerParams = std::make_shared<std::mutex>();
		}

		//////
		// initialize edit history parameters
		//////
		{
			editHistoryParams.editHistoryVec.clear();
			editHistoryParams.editHistoryVec.push_back(editHistory({std::vector<std::string>({}), nlohmann::json::object()}));
			editHistoryParams.editHistoryIndex = 0;
			editHistoryParams.mutexEditHistoryParams = std::make_shared<std::mutex>();
		}

		//////
		// initialize user interface parameters
		//////
		{
			interfaceParams.cameraTarget << 0, 0, 0;
			interfaceParams.cameraPos << -30, 40, 30;
			interfaceParams.cameraUp << 0, 1, 0;
			interfaceParams.cameraZoom = 1.0;
			interfaceParams.mutexInterfaceParams = std::make_shared<std::mutex>();
		}

		////
		// initialize mesh data parameter
		////
		{
			mutexMeshes = std::make_shared<std::mutex>();
		}

		////
		// initialize logger for this room
		////
		{
			logger.initialize(UUID, core->config);
			{
				std::stringstream s;
				s << "New room \"" << UUID << "\" is created.";
				logger.log(s.str(), "SYSTEM");
			}
		}

		////
		// initialize outputDir for this room
		////
		{
			const std::string roomCreatedTimeStamp = Logger::getCurrentTimestampAsString(false);
			std::stringstream tmp;
			tmp << roomCreatedTimeStamp;
			tmp << "-";
			tmp << UUID;
			const std::string timestampAndUUID = tmp.str();

			outputDir = core->config.at("outputsDir").get<std::string>();
			outputDir.append(timestampAndUUID);
			outputDir.make_preferred();
			fs::create_directories(outputDir);
		}
	}
} // namespace

#endif
