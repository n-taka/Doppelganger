#ifndef LOG_H
#define LOG_H

#include <string>
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

#include <nlohmann/json.hpp>

namespace Doppelganger
{
	namespace Util
	{
		void log(
			const std::string &content,
			const std::string &level,
			const nlohmann::json& config);
		void log(
			const fs::path &path,
			const std::string &level,
			const nlohmann::json& config);
	}
}

#endif
