#ifndef ROOM_CPP
#define ROOM_CPP

#include "Room.h"

// #include "util/loadTexture.h"
// #include "util/projectMeshAttributes.h"
// #include "util/readWriteMeshFromToFile.h"
// #include "util/saveTexture.h"
// #include "util/updateMeshesFromJson.h"
// #include "util/writeBase64ToFile.h"
// #include "util/writeMeshToJson.h"
// #include "util/writeMeshToMemory.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace Doppelganger
{
	Room::Room(const std::string &UUID_) : UUID(UUID_)
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
	}
} // namespace Doppel

#endif
