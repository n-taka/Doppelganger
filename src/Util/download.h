#ifndef DOWNLOAD_H
#define DOWNLOAD_H

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
		bool download(const std::shared_ptr<Core> &core, const std::string &targetUrl, const fs::path &destPath);
	};
} // namespace

#endif
