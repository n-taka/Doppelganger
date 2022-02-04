#ifndef GETPLUGINCATALOGUE_H
#define GETPLUGINCATALOGUE_H

#if defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#elif defined(__APPLE__)
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#elif defined(__linux__)
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include <string>
#include <nlohmann/json.hpp>

namespace Doppelganger
{
	namespace Util
	{
		void getPluginCatalogue(
			const fs::path &pluginDir,
			const nlohmann::json &listURLJsonArray,
			nlohmann::json &catalogue);
	}
}

#endif
