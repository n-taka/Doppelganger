#ifndef UTIL_H
#define UTIL_H

#include <memory>
#if defined(_WIN32) || defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#endif

namespace Doppelganger
{
	class Core;

	namespace Util
	{
		void download(const std::shared_ptr<Core> &core, const std::string &targetUrl, const fs::path &destPath);
		void unzip(const std::shared_ptr<Core> &core, const fs::path &zipPath, const fs::path &destPath);
	};
} // namespace

#endif
