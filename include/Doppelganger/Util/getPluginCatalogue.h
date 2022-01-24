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
#include <vector>
#include <nlohmann/json.hpp>

namespace Doppelganger
{
	namespace Util
	{
		void getPluginCatalogue(
			const fs::path &pluginDir,
			const std::vector<std::string> &listURLList,
			nlohmann::json &catalogue);
	}
}

#endif
