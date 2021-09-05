#ifndef UNZIP_H
#define UNZIP_H

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
		void unzip(const std::shared_ptr<Core> &core, const fs::path &zipPath, const fs::path &destPath);
	};
} // namespace

#endif
