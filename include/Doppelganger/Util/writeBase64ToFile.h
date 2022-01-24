#ifndef WRITEBASE64TOFILE_H
#define WRITEBASE64TOFILE_H

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
		void writeBase64ToFile(const std::string &base64Str, const fs::path &destPath);
	};
}

#endif
