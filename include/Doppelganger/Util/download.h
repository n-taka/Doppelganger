#ifndef DOWNLOAD_H
#define DOWNLOAD_H

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
		bool download(const std::string &targetUrl, const fs::path &destPath);
	};
} // namespace

#endif
