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
	namespace Util
	{
		void unzip(const fs::path &zipPath, const fs::path &destPath);
	};
} // namespace

#endif
