#ifndef LOG_H
#define LOG_H

#include <string>
#include <unordered_map>
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

namespace Doppelganger
{
	namespace Util
	{
		struct LogConfig
		{
			std::unordered_map<std::string, bool> level;
			std::unordered_map<std::string, bool> type;
		};

		void log(
			const std::string &content,
			const std::string &level,
			const fs::path &dataDir,
			const LogConfig& logConfig);
		void log(
			const fs::path &path,
			const std::string &level,
			const fs::path &dataDir,
			const LogConfig& logConfig);
	}
}

#endif
